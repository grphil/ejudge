/* -*- mode: c; coding: koi8-r -*- */
/* $Id$ */

/* Copyright (C) 2000,2001 Alexander Chernov <cher@ispras.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "runlog.h"
#include "parsecfg.h"
#include "teamdb.h"
#include "prepare.h"
#include "html.h"
#include "clarlog.h"

#include "misctext.h"
#include "base64.h"
#include "pathutl.h"
#include "fileutl.h"
#include "xalloc.h"
#include "logger.h"
#include "osdeps.h"

#include <time.h>
#include <stdio.h>
#include <string.h>

#if CONF_HAS_LIBINTL - 0 == 1
#include <libintl.h>
#define _(x) gettext(x)
#else
#define _(x) x
#endif

/* max. packet size */
#define MAX_PACKET_SIZE 256
typedef char packet_t[MAX_PACKET_SIZE];

static unsigned long current_time;

static unsigned long contest_start_time;
static unsigned long contest_sched_time;
static unsigned long contest_duration;
static unsigned long contest_stop_time;

struct server_cmd
{
  char const  *cmd;
  int        (*func)(char const *, packet_t const, void *);
  void        *ptr;
};

void
update_standings_file(void)
{
  time_t cur_time = time(0);
  time_t start_time, stop_time, duration, start_fog_time;

  run_get_times(&start_time, 0, &duration, &stop_time);
  start_fog_time = start_time + duration - global->board_fog_time;

  if (start_time) ASSERT(cur_time >= start_time);
  if (stop_time)  ASSERT(cur_time >= stop_time);
  if (stop_time && cur_time <= stop_time + global->board_unfog_time) return;
  if (start_time && cur_time >= start_fog_time) return;

  write_standings(global->status_dir, "standings.html");
}

int
update_status_file(int force_flag)
{
  static time_t prev_status_update = 0;
  time_t cur_time;
  char buf[256];

  unsigned long start_time;
  unsigned long sched_time;
  unsigned long duration;
  unsigned long stop_time;
  int           total_runs;
  int           total_clars;

  cur_time = time(0);
  if (!force_flag && cur_time <= prev_status_update) return 0;

  run_get_times(&start_time, &sched_time, &duration, &stop_time);
  total_runs = run_get_total();
  total_clars = clar_get_total();
  sprintf(buf, "%lu %lu %lu %lu %lu %d %d\n",
          cur_time, start_time, sched_time, duration, stop_time,
          total_runs, total_clars);
  generic_write_file(buf, strlen(buf), SAFE,
                     global->status_dir, "status", "");
  prev_status_update = cur_time;
  return 1;
}

int
check_team_quota(int teamid, unsigned int size)
{
  int num;
  unsigned long total;

  if (size > global->max_run_size) return -1;
  run_get_team_usage(teamid, &num, &total);
  if (num > global->max_run_num || total + size > global->max_run_total)
    return -1;
  return 0;
}

int
check_clar_qouta(int teamid, unsigned int size)
{
  int num;
  unsigned long total;

  if (size > global->max_clar_size) return -1;
  clar_get_team_usage(teamid, &num, &total);
  if (num > global->max_clar_num || total + size > global->max_clar_total)
    return -1;
  return 0;
}

int
report_to_client(char const *pk_name, char const *str)
{
  if (str) {
    generic_write_file(str, strlen(str), PIPE,
                       global->pipe_dir, pk_name, "");
  }
  return 0;
}

int
report_bad_packet(char const *pk_name, int rm_mode)
{
  char buf[1024];

  err(_("bad packet"));
  sprintf(buf,
          "<h2>%s</h2><p>%s</p>",
          _("Server is unable to perform your request"),
          _("Misformed request"));

  report_to_client(pk_name, buf);
  if (rm_mode == 1) relaxed_remove(global->team_data_dir, pk_name);
  if (rm_mode == 2) relaxed_remove(global->judge_data_dir, pk_name);

  return 0;
}

int
report_ok(char const *pk_name)
{
  char *msg = "OK";

  report_to_client(pk_name, msg);
  return 0;
}

int
check_period(char const *pk_name, char const *func, char const *extra,
             int before, int during, int after)
{
  char *s = 0;
  char *t = 0;
  if (!contest_start_time) {
    /* before the contest */
    if (!before) {
      s = _("contest is not started");
      //t = "<p>������������ ��� �� ��������.";
      t = _("<p>The contest is not started.");
      goto _failed;
    }
  } else if (!contest_stop_time) {
    /* during the contest */
    if (!during) {
      s = _("contest is already started");
      //t = "<p>������������ ��� ��������.";
      t = _("<p>The contest is already started.");
      goto _failed;
    }
  } else {
    /* after the contest */
    if (!after) {
      s = _("contest is stopped");
      //t = "<p>������������ ��� �����������.";
      t = _("<p>The contest is already over.");
      goto _failed;
    }
  }
  return 0;

 _failed:
  {
    int len = 0;
    char *buf, *p;

    if (func) len += strlen(func) + 2;
    if (extra) len += strlen(extra) + 2;
    len += strlen(s);

    buf = p = alloca(len + 4);
    buf[0] = 0;
    if (func)  p += sprintf(p, "%s: ", func);
    if (extra) p += sprintf(p, "%s: ", extra);
    sprintf(p, "%s", s);
    err(buf);

    report_to_client(pk_name, t);
  }
  return -1;
}

int
team_view_clar(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd;
  int  team_id, clar_id, n;

  if (sscanf(pk_str, "%s %d %d %n", cmd, &team_id, &clar_id, &n) != 3)
    return report_bad_packet(pk_name ,1);
  if (pk_str[n] || !teamdb_lookup(team_id))
    return report_bad_packet(pk_name, 1);
  if (clar_id < 0 || clar_id >= clar_get_total())
    return report_bad_packet(pk_name, 1);
  write_team_clar(team_id, clar_id,
                  global->clar_archive_dir, global->pipe_dir, pk_name);
  return 0;
}

int
team_send_clar(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd, subj, ip;
  int  team, n;

  char *msg = 0;
  int   rsize = 0;
  char *reply = 0;
  int   clar_id;
  char  clar_name[64];

  if (sscanf(pk_str, "%s %d %s %s %n", cmd, &team, subj, ip, &n) != 4)
    return report_bad_packet(pk_name, 1);
  if (pk_str[n] || !teamdb_lookup(team))
    return report_bad_packet(pk_name, 1);
  if (strlen(subj) > CLAR_MAX_SUBJ_LEN)
    return report_bad_packet(pk_name, 1);
  if (strlen(ip) > RUN_MAX_IP_LEN)
    return report_bad_packet(pk_name, 1);

  /* disallow sending messages from team before and after the contest */
  if (check_period(pk_name, "team_send_clar", teamdb_get_login(team),
                   0, 1, 0) < 0) {
    relaxed_remove(global->team_data_dir, pk_name);
    return 0;
  }

  if (generic_read_file(&msg, 0, &rsize, REMOVE,
                        global->team_data_dir, pk_name, "") < 0) {
    //reply = "<p>������ �� ���� ������� ����� ���������.";
    reply = _("<p>Server failed to read the message body.");
    goto report_to_client;
  }

  if (check_clar_qouta(team, rsize) < 0) {
    //reply = "<p>��������� �� ����� ���� �������: �������� ����� ������� ��� �������.";
    reply = _("<p>The message cannot be sent. Message quota exceeded for this team.");
    goto report_to_client;
  }

  /* update log */
  if ((clar_id = clar_add_record(time(0), rsize, ip,
                                 team, 0, 0, subj)) < 0) {
    //reply = "<p>��������� �� ��������. ������ ���������� ������� ���������.";
    reply = _("<p>The message is not sent. Error while updating message log.");
    goto report_to_client;
  }

  /* write this request to base */
  sprintf(clar_name, "%06d", clar_id);
  if (generic_write_file(msg, rsize, 0,
                         global->clar_archive_dir, clar_name, "") < 0) {
    //reply = "<p>��������� �� ��������. ������ ������ ��������� � �����.";
    reply = _("<p>The message is not sent. Failed to write the message to the archive.");
    goto report_to_client;
  }

  //reply = "<p>��������� ��������.";
  reply = _("<p>The message is sent.");

 report_to_client:
  if (reply) {
    generic_write_file(reply, strlen(reply), PIPE,
                       global->pipe_dir, pk_name, "");
  }
  xfree(msg);
  return 0;
} 

int
team_submit(char const *pk_name, const packet_t pk_str, void *ptr)
{
  char cmd[256];
  char ip[256];
  int  team, prob, lang, n;
  char *reply = 0;
  char *src = 0;
  int   src_len = 0;
  int   run_id;
  char  run_name[64];
  char  run_full[64];
  int   needs_remove = 1;

  if (sscanf(pk_str, "%s %d %d %d %s %n",
             cmd, &team, &prob, &lang, ip, &n) != 5
      || pk_str[n]
      || strlen(ip) > RUN_MAX_IP_LEN
      || !teamdb_lookup(team)
      || lang < 1 || lang > max_lang
      || !(langs[lang])
      || prob < 1 || prob > max_prob
      || !(probs[prob]))
    return report_bad_packet(pk_name, 1);

  /* disallow submissions out of contest time */
  if (check_period(pk_name, "SUBMIT", teamdb_get_login(team), 0, 1, 0) < 0)
    goto _cleanup;

  /* this looks like a valid packet */
  /* try to read source file */
  if (generic_read_file(&src, 0, &src_len, REMOVE,
                        global->team_data_dir, pk_name, "") < 0) {
    //reply = "<p>������ �� ���� ������� ����� ���������.";
    reply = _("<p>Server failed to read the program source.");
    goto report_to_client;
  }

  /* the last generic_read_file should remove the data file */
  needs_remove = 0;

  /* check the limits */
  if (check_team_quota(team, src_len) < 0) {
    //reply = "<p>������� �� ����� ���� �������: �������� ����� �������";
    reply = _("<p>The submission cannot be accepted. Quota exceeded.");
    err(_("team %d:run quota exceeded"), team);
    goto report_to_client;
  }

    /* now save the source and create a log record */
  if ((run_id = run_add_record(time(NULL),src_len,ip,team, prob, lang)) < 0){
    //reply = "<p>������ �� ���� �������� ������ �������.";
    reply = _("<p>Server failed to update submission log.");
    goto report_to_client;
  }

  sprintf(run_name, "%06d", run_id);
  sprintf(run_full, "%06d%s", run_id, langs[lang]->src_sfx);

  if (generic_write_file(src, src_len, 0,
                         global->run_archive_dir, run_name, "") < 0) {
    //reply = "<p>������ �� ���� ��������� ��������� � ������.";
    reply = _("<p>Server failed to save the program in the archive.");
    goto report_to_client;
  }
  if (generic_write_file(src, src_len, SAFE,
                         langs[lang]->src_dir, run_name,
                         langs[lang]->src_sfx) < 0) {
    //reply = "<p>������ �� ���� �������� ��������� ��� ����������.";
    reply = _("<p>Server failed to pass the program for compilation.");
    goto report_to_client;
  }

  if (run_change_status(run_id, RUN_COMPILING, 0) < 0) {
    //reply = "<p>������ �� ���� �������� ������ �������.";
    reply = _("<p>Server failed to update submission log.");
    goto report_to_client;
  }
  //reply = "<p>������� �������� �� ��������.";
  reply = _("<p>Submission is sent.");
  
 report_to_client:
  if (reply) {
    generic_write_file(reply, strlen(reply), PIPE,
                       global->pipe_dir, pk_name, "");
  }

 _cleanup:
  if (needs_remove) relaxed_remove(global->team_data_dir, pk_name);
  xfree(src);
  return 0;
}

int
team_change_passwd(char const *pk_name, const packet_t pk_str, void *ptr)
{
  char  cmd[256];
  char  passwd[256];
  int   team_id, n;
  char *reply = 0;

  if (sscanf(pk_str, "%s %d %s %n", cmd, &team_id, passwd, &n) != 3
      || pk_str[n]
      || !teamdb_lookup(team_id)
      || strlen(passwd) > TEAMDB_MAX_SCRAMBLED_PASSWD_SIZE)
    report_bad_packet(pk_name, 0);

  if (!teamdb_set_scrambled_passwd(team_id, passwd)) {
    //reply = "<p>����� ������ �� ����� ���� ����������.";
    reply = _("<p>New password cannot be set.");
    goto report_to_client;
  }
  if (teamdb_write_passwd(global->passwd_file) < 0) {
    //reply = "<p>����� ������ �� ����� ���� ��������.";
    reply = _("<p>New password cannot be saved.");
    goto report_to_client;
  }

  //reply = "<p>������ ������� �������.";
  reply = _("<p>Password is changed successfully.");

 report_to_client:
  if (reply) {
    generic_write_file(reply, strlen(reply), PIPE,
                       global->pipe_dir, pk_name, "");
  }
  return 0;
}

int
team_stat(char const *pk_name, packet_t const pk_str, void *ptr)
{
  packet_t cmd;
  int      team, p1, p2, n;

  if (sscanf(pk_str, "%s %d %d %d %n", cmd, &team, &p1, &p2, &n) != 4
      || pk_str[n] || !teamdb_lookup(team))
    return report_bad_packet(pk_name, 0);

  write_team_statistics(team, p1, p2, global->pipe_dir, pk_name);
  return 0;
}

struct server_cmd team_commands[]=
{
  { "SUBMIT", team_submit, 0 },
  { "STAT", team_stat, 0 },
  { "PASSWD", team_change_passwd, 0 },
  { "VIEW", team_view_clar, 0 },
  { "CLAR", team_send_clar, 0 },
  { 0, 0, 0 }
};

int
read_team_packet(char const *pk_name)
{
  packet_t  pk_str, cmd;
  char     *pbuf = pk_str;
  int       i, r, rsize;

  memset(pk_str, 0, sizeof(pk_str));
  r = generic_read_file(&pbuf, sizeof(pk_str), &rsize, SAFE|REMOVE,
                        global->team_cmd_dir, pk_name, "");
  if (r == 0) return 0;
  if (r < 0) return -1;

  info(_("packet: %s"), chop(pk_str));
  sscanf(pk_str, "%s", cmd);
  for (i = 0; team_commands[i].cmd; i++) {
    if (!strcmp(team_commands[i].cmd, cmd))
      return team_commands[i].func(pk_name, pk_str, team_commands[i].ptr);
  }
  report_bad_packet(pk_name, 0);
  return 0;
}

int
read_compile_packet(char *pname)
{
  char   buf[256];
  char   exe_name[64];

  int  r, n;
  int  rsize;
  int  code;
  int  runid;
  int  cn;

  int  lang, prob, stat;

  char *pbuf = buf;

  memset(buf, 0, sizeof(buf));
  r = generic_read_file(&pbuf, sizeof(buf), &rsize, SAFE|REMOVE,
                        global->compile_status_dir, pname, "");
  if (r == 0) return 0;
  if (r < 0) return -1;

  info(_("compile packet: %s"), chop(buf));
  if (sscanf(pname, "%d%n", &runid, &n) != 1 || pname[n])
    goto bad_packet_error;
  if (sscanf(buf, "%d %n", &code, &n) != 1 || buf[n])
    goto bad_packet_error;
  if (run_get_param(runid, &lang, &prob, &stat) < 0)
    goto bad_packet_error;
  if (stat != RUN_COMPILING) goto bad_packet_error;
  if (code != RUN_OK && code != RUN_COMPILE_ERR) goto bad_packet_error;
  if (code == RUN_COMPILE_ERR) {
    /* compilation error */
    if (run_change_status(runid, RUN_COMPILE_ERR, 0) < 0) return -1;
    if (generic_copy_file(REMOVE, global->compile_report_dir, pname, "",
                          0, global->report_archive_dir, pname, "") < 0)
      return -1;
    update_standings_file();
    return 1;
  }
  if (run_change_status(runid, RUN_COMPILED, 0) < 0) return -1;

  /* find appropriate checker */
  cn = find_tester(prob, langs[lang]->arch);
  ASSERT(cn >= 1 && cn <= max_tester && testers[cn]);

  /* copy the executable into the testers's queue */
  sprintf(exe_name, "%06d%s", runid, langs[lang]->exe_sfx);
  if (generic_copy_file(REMOVE, global->compile_report_dir, exe_name, "",
                        SAFE, testers[cn]->exe_dir, exe_name, "") < 0)
    return -1;

  /* update status */
  if (run_change_status(runid, RUN_RUNNING, 0) < 0) return -1;

  return 1;

 bad_packet_error:
  err(_("bad_packet"));
  return 0;
}

int
read_run_packet(char *pname)
{
  char  buf[256];
  char *pbuf = buf;
  int   r, rsize, n;

  int   runid;
  int   status;
  int   test;

  int   log_stat, log_prob, log_lang;

  memset(buf, 0 ,sizeof(buf));
  r = generic_read_file(&pbuf, sizeof(buf), &rsize, SAFE|REMOVE,
                        global->run_status_dir, pname, "");
  if (r < 0) return -1;
  if (r == 0) return 0;

  info(_("run packed: %s"), chop(buf));
  if (sscanf(pname, "%d%n", &runid, &n) != 1 || pname[n])
    goto bad_packet_error;
  if (sscanf(buf, "%d%d %n", &status, &test, &n) != 2 || buf[n])
    goto bad_packet_error;
  if (run_get_param(runid, &log_lang, &log_prob, &log_stat) < 0)
    goto bad_packet_error;
  if (log_stat != RUN_RUNNING) goto bad_packet_error;
  if (status<0 || status>RUN_CHECK_FAILED || test<0) goto bad_packet_error;
  if (run_change_status(runid, status, test) < 0) return -1;
  update_standings_file();
  if (generic_copy_file(REMOVE, global->run_report_dir, pname, "",
                        0, global->report_archive_dir, pname, "") < 0)
    return -1;
  return 1;

 bad_packet_error:
  err(_("bad_packet"));
  return 0;
}

void
process_judge_reply(int clar_ref, int to_all,
                    char const *pname, char const *ip)
{
  char *txt = 0;
  int   tsize = 0;
  int   from;
  char *stxt = 0;
  int   ssize = 0;
  char  name1[64];
  char  name2[64];
  char *newsubj = 0;
  int   newsubjlen;
  char *fullmsg = 0;
  int   fullmsglen;
  char  codedsubj[CLAR_MAX_SUBJ_LEN + 4];
  int   to, newclar;
  int   qsize;
  char *qbuf;

  if (generic_read_file(&txt, 0, &tsize, REMOVE,
                        global->judge_data_dir, pname, "") < 0)
    goto exit_notok;
  if (clar_get_record(clar_ref, 0, 0, 0, &from, 0, 0, 0) < 0)
    goto exit_notok;
  if (!from) {
    err(_("cannot reply to judge's message %d"), clar_ref);
    goto exit_notok;
  }
  sprintf(name1, "%06d", clar_ref);
  if (generic_read_file(&stxt, 0, &ssize, 0,
                        global->clar_archive_dir, name1, "") < 0)
    goto exit_notok;
  newsubj = alloca(ssize + 64);
  newsubjlen = message_reply_subj(stxt, newsubj);
  message_base64_subj(newsubj, codedsubj, CLAR_MAX_SUBJ_TXT_LEN);
  ASSERT(strlen(codedsubj) <= CLAR_MAX_SUBJ_LEN);
  qsize = message_quoted_size(stxt);
  qbuf = alloca(qsize + 16);
  fullmsg = alloca(tsize + qsize + newsubjlen + 64);
  message_quote(stxt, qbuf);
  //fprintf(stderr, ">>%s<<\n>>%s<<\n", newsubj, qbuf);
  strcpy(fullmsg, newsubj);
  strcat(fullmsg, qbuf);
  strcat(fullmsg, "\n");
  strcat(fullmsg, txt);
  fullmsglen = strlen(fullmsg);
  to = 0;
  if (!to_all) to = from;

  /* create new clarid */
  info(_("coded (%d): %s"), strlen(codedsubj), codedsubj);
  if ((newclar = clar_add_record(time(0), fullmsglen,
                                 ip, 0, to, 0, codedsubj)) < 0)
    goto exit_notok;
  sprintf(name2, "%06d", newclar);
  generic_write_file(fullmsg, fullmsglen, 0,
                     global->clar_archive_dir, name2, "");
  clar_update_flags(clar_ref, 2);
  xfree(txt);
  generic_write_file("OK\n", 3, PIPE,
                     global->pipe_dir, pname, "");
  return;

 exit_notok:
  xfree(txt);
  generic_write_file("NOT OK\n", 7, PIPE,
                     global->pipe_dir, pname, "");
}

void
rejudge_run(int run_id)
{
  int lang;
  char run_name[64];

  if (run_get_record(run_id, 0, 0, 0, 0, &lang, 0, 0, 0) < 0) return;
  if (lang <= 0 || lang > max_lang || !langs[lang]) {
    err(_("rejudge_run: bad language: %d"), lang);
    return;
  }

  sprintf(run_name, "%06d", run_id);
  if (generic_copy_file(0, global->run_archive_dir, run_name, "",
                        SAFE, langs[lang]->src_dir, run_name,
                        langs[lang]->src_sfx) < 0)
    return;

  run_change_status(run_id, RUN_COMPILING, 0);
}

int
judge_stat(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd;
  int      all_runs_flag = 0;
  int      all_clars_flag = 0;
  int      master_mode = (int) ptr;
  int      n;

  if (sscanf(pk_str, "%s %d %d %n", cmd,
             &all_runs_flag, &all_clars_flag, &n) != 3
      || pk_str[n])
    return report_bad_packet(pk_name, 0);

  write_judge_allstat(master_mode,
                      all_runs_flag, all_clars_flag,
                      global->pipe_dir, pk_name);
  return 1;
}

int
judge_standings(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd;
  int      n;

  if (sscanf(pk_str, "%s %n", cmd, &n) != 1 || pk_str[n])
    return report_bad_packet(pk_name, 0);
  write_judge_standings(pk_name);
  return 0;
}

int
judge_view_clar(char const *pk_name, const packet_t pk_str, void *ptr)
{
  int      is_master = (int) ptr;
  packet_t cmd;
  int      c_id, n, flags = 0;

  if (sscanf(pk_name, "%s %d %n", cmd, &c_id, &n) != 2 || pk_str[n]
      || c_id < 0 || c_id >= clar_get_total())
    return report_bad_packet(pk_name, 0);

  if (is_master) {
    write_clar_view(c_id, global->clar_archive_dir,
                    global->pipe_dir, pk_name, 0);
  } else {
    write_clar_view(c_id, global->clar_archive_dir,
                      global->pipe_dir, pk_name, 0);
    clar_get_record(c_id, 0, 0, 0, 0, 0, &flags, 0);
    if (!flags) flags = 1;
    clar_update_flags(c_id, flags);
  }
  return 0;
}

int
judge_view_report(char const *pk_name, const packet_t pk_str, void *ptr)
{
  int      n, rid;
  packet_t cmd;

  if (sscanf(pk_str, "%s %d %n", cmd, &rid, &n) != 2
      || pk_str[n]
      || rid < 0 || rid >= run_get_total())
    return report_bad_packet(pk_name, 0);

  write_judge_report_view(pk_name, rid);
  return 0;
}

int
judge_view_src(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t  cmd;
  int       rid, n;

  if (sscanf(pk_str, "%s %d %n", cmd, &rid, &n) != 2
      || pk_str[n]
      || rid < 0 || rid >= run_get_total())
    return report_bad_packet(pk_name, 0);

  write_judge_source_view(pk_name, rid);
  return 0;
}

int
judge_start(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd;
  int      n;
  time_t   ts;

  if(sscanf(pk_str, "%s %n", cmd, &n) != 1 || pk_str[n])
    return report_bad_packet(pk_name, 0);
  if (check_period(pk_name, "START", 0, 1, 0, 0) < 0) return 0;

  run_start_contest(time(&ts));
  contest_start_time = ts;
  info(_("contest started: %lu"), ts);
  update_standings_file();
  update_status_file(1);
  report_ok(pk_name);
  return 0;
}

int
judge_stop(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd;
  int      n;
  time_t   ts;

  if (sscanf(pk_str, "%s %n", cmd, &n) != 1 || pk_str[n])
    return report_bad_packet(pk_name, 0);
  if (check_period(pk_name, "STOP", 0, 1, 1, 0) < 0) return 0;

  run_stop_contest(time(&ts));
  contest_stop_time = ts;
  info(_("contest stopped: %lu"), ts);
  update_standings_file();
  update_status_file(1);
  report_ok(pk_name);
  return 0;
}

int
judge_sched(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t  cmd;
  int       n;
  time_t    newtime;
  char     *reply = "OK";

  if (sscanf(pk_str, "%s %lu %n", cmd, &newtime, &n) != 2 || pk_str[n])
    return report_bad_packet(pk_name, 0);
  if (check_period(pk_name, "SCHED", 0, 1, 0, 0) < 0) return 0;

  run_sched_contest(newtime);
  contest_sched_time = newtime;
  info(_("contest scheduled: %lu"), newtime);
  update_standings_file();
  update_status_file(1);

  report_to_client(pk_name, reply);
  return 0;
}

int
judge_time(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t  cmd;
  int       newtime, n;
  char     *reply = "OK";

  if (sscanf(pk_str, "%s %d %n", cmd, &newtime, &n) != 2 ||
      pk_str[n] || newtime <= 0 || newtime > 60 * 10)
    return report_bad_packet(pk_name, 0);

  if (check_period(pk_name, "TIME", 0, 1, 1, 0) < 0) return 0;
  if (newtime * 60 < global->contest_time) {
    err(_("contest time cannot be decreased"));
    //reply = "<p>����� ������������ �� ����� ���� ���������.";
    reply = _("<p>The contest time cannot be decreased.");
    goto _cleanup;
  }

  contest_duration = newtime * 60;
  run_set_duration(contest_duration);
  info(_("contest time reset to %d"), newtime);
  update_standings_file();
  update_status_file(1);

 _cleanup:
  report_to_client(pk_name, reply);
  return 0;  
}

int
judge_change_status(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd;
  int      runid, status, test, n;

  if (sscanf(pk_str, "%s %d %d %d %n", cmd, &runid, &status, &test, &n) != 4
      || pk_str[n]
      || runid < 0 || runid >= run_get_total()
      || status < 0 || status > RUN_REJUDGE
      || (status > RUN_WRONG_ANSWER_ERR && status < RUN_REJUDGE)
      || test < 0 || test > 99)
    return report_bad_packet(pk_name, 0);

  run_change_status(runid, status, test);
  if (status == RUN_REJUDGE) {
    rejudge_run(runid);
  }

  report_ok(pk_name);
  return 0;
}

int
judge_reply(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd, ip;
  int      c_id, toall, n;

  if (sscanf(pk_str, "%s %d %d %s %n", cmd, &c_id, &toall, ip, &n) != 4
      || pk_str[n]
      || c_id < 0 || c_id >= clar_get_total()
      || strlen(ip) > RUN_MAX_IP_LEN)
    return report_bad_packet(pk_name, 2);

  process_judge_reply(c_id, toall, pk_name, ip);
  return 0;
}

int
judge_message(char const *pk_name, const packet_t pk_str, void *ptr)
{
  packet_t cmd, subj, ip, c_name;
  char *msg = 0;
  int   mlen, n;
  char *reply = "OK";
  int   c_id;

  if (sscanf(pk_str, "%s %s %s %n", cmd, subj, ip, &n) != 3
      || pk_str[n]
      || strlen(subj) > CLAR_MAX_SUBJ_LEN
      || strlen(ip) > RUN_MAX_IP_LEN)
    return report_bad_packet(pk_name, 2);

  if (generic_read_file(&msg, 0, &mlen, REMOVE,
                        global->judge_data_dir, pk_name, "") < 0) {
    //reply = "<p>������ �� ����� ��������� ���� � ������� ���������.";
    reply = _("<p>Server failed to read the message file.");
    goto _cleanup;
  }

  if ((c_id = clar_add_record(time(0), mlen, ip, 0, 0, 0, subj)) < 0) {
    //reply = "<p>������ �� ����� �������� ������ ���������.";
    reply = _("<p>Server failed to update message log.");
    goto _cleanup;
  }

  sprintf(c_name, "%06d", c_id);
  if (generic_write_file(msg, mlen, 0,
                         global->clar_archive_dir, c_name, "") < 0) {
    //reply = "<p>������ �� ����� ��������� ���������.";
    reply = _("<p>Server failed to save the message.");
  }

 _cleanup:
  report_to_client(pk_name, reply);
  xfree(msg);
  return 0;
}

struct server_cmd judge_cmds[] =
{
  { "START", judge_start, 0 },
  { "STOP", judge_stop, 0 },
  { "SCHED", judge_sched, 0 },
  { "TIME", judge_time, 0 },
  { "MSTAT", judge_stat, (void*) 1 },
  { "JSTAT", judge_stat, 0},
  { "CHGSTAT", judge_change_status, 0 },
  { "SRC", judge_view_src, 0 },
  { "REPORT", judge_view_report, 0 },
  { "MPEEK", judge_view_clar, (void*) 1 },
  { "JPEEK", judge_view_clar, 0 },
  { "REPLY", judge_reply, 0 },
  { "MSG", judge_message, 0 },
  { "STAND", judge_standings, 0 },

  { 0, 0, 0 },
};

int
read_judge_packet(char const *pk_name)
{
  int       rsize, i, r;
  packet_t  pk_str, cmd;
  char     *pbuf = pk_str;

  memset(pk_str, 0, sizeof(pk_str));
  r = generic_read_file(&pbuf, sizeof(pk_str), &rsize, SAFE|REMOVE,
                        global->judge_cmd_dir, pk_name, "");
  if (r == 0) return 0;
  if (r < 0) return -1;

  info(_("judge packet: %s"), chop(pk_str));
  sscanf(pk_str, "%s", cmd);
  for (i = 0; judge_cmds[i].cmd; i++)
    if (!strcmp(judge_cmds[i].cmd, cmd))
      return judge_cmds[i].func(pk_name, pk_str, judge_cmds[i].ptr);
  report_bad_packet(pk_name, 2);
  return 0;
}

int
do_loop(void)
{
  path_t packetname;
  int    r;

  update_standings_file();

  run_get_times(&contest_start_time, &contest_sched_time,
                &contest_duration, &contest_stop_time);
  if (!contest_duration) {
    contest_duration = global->contest_time;
    run_set_duration(contest_duration);
  }

  while (1) {
    while (1) {
      /* update current time */
      current_time = time(0);

      /* check stop and start times */
      if (contest_start_time && !contest_stop_time) {
        if (current_time >= contest_start_time + contest_duration) {
          /* the contest is over! */
          info(_("CONTEST OVER"));
          run_stop_contest(contest_start_time + contest_duration);
          contest_stop_time = contest_start_time + contest_duration;
        }
      } else if (contest_sched_time && !contest_start_time) {
        if (current_time >= contest_sched_time) {
          /* it's time to start! */
          info(_("CONTEST STARTED"));
          run_start_contest(current_time);
          contest_start_time = current_time;
        }
      }

      /* indicate, that we're alive, and do it somewhat quiet  */
      logger_set_level(-1, LOG_WARNING);
      update_status_file(0);
      logger_set_level(-1, 0);

      r = scan_dir(global->team_cmd_dir, packetname);
      if (r < 0) return -1;
      if (r > 0) {
        if (read_team_packet(packetname) < 0) return -1;
        break;
      }

      r = scan_dir(global->judge_cmd_dir, packetname);
      if (r < 0) return -1;
      if (r > 0) {
        if (read_judge_packet(packetname) < 0) return -1;
        break;
      }

      r = scan_dir(global->compile_status_dir, packetname);
      if (r < 0) return -1;
      if (r > 0) {
        if (read_compile_packet(packetname) < 0) return -1;
        break;
      }

      r = scan_dir(global->run_status_dir, packetname);
      if (r < 0) return -1;
      if (r > 0) {
        if (read_run_packet(packetname) < 0) return -1;
        break;
      }
    
      //write_log(0, LOG_INFO, "no new packets");
      os_Sleep(global->serve_sleep_time);
    }
  }
}

static int
write_submit_templates(char const *status_dir)
{
  char  buf[1024];
  char *s;
  int   i;

  /* generate problem selection control */
  s = buf + sprintf(buf, "<select name=\"problem\">"
                    "<option value=\"\">\n");
  for (i = 1; i <= max_prob; i++)
    if (probs[i])
      s += sprintf(s, "<option value=\"%d\">%s - %s\n",
                   probs[i]->id, probs[i]->short_name, probs[i]->long_name);
  sprintf(s, "</select>\n");
  if (generic_write_file(buf,strlen(buf),SAFE,status_dir,"problems","") < 0)
    return -1;

  /* generate problem2 selection control */
  s = buf + sprintf(buf, "<select name=\"problem\">"
                    "<option value=\"\">\n");
  for (i = 1; i <= max_prob; i++)
    if (probs[i])
      s += sprintf(s, "<option value=\"%s\">%s - %s\n",
                   probs[i]->short_name,
                   probs[i]->short_name, probs[i]->long_name);
  sprintf(s, "</select>\n");
  if (generic_write_file(buf,strlen(buf),SAFE,status_dir,"problems2","") < 0)
    return -1;

  /* generate language selection control */
  s = buf + sprintf(buf, "<select name=\"language\">"
                    "<option value=\"\">\n");
  for (i = 1; i <= max_lang; i++)
    if (langs[i])
      s += sprintf(s, "<option value=\"%d\">%s - %s\n",
                   langs[i]->id, langs[i]->short_name, langs[i]->long_name);
  sprintf(s, "</select>\n");
  if (generic_write_file(buf,strlen(buf),SAFE,status_dir,"languages","") < 0)
    return -1;

  return 0;
}

int
main(int argc, char *argv[])
{
  path_t  cpp_opts = { 0 };
  int     code = 0;
  int     p_flags = 0;
  int     i = 1;

  if (argc == 1) goto print_usage;
  code = 1;

  while (i < argc) {
    if (!strncmp(argv[i], "-D", 2)) {
      if (cpp_opts[0]) pathcat(cpp_opts, " ");
      pathcat(cpp_opts, argv[i++]);
    } else if (!strcmp(argv[i], "-E")) {
      i++;
      p_flags |= PREPARE_USE_CPP;
    } else break;
  }
  if (i >= argc) goto print_usage;

  if (prepare(argv[i], p_flags, PREPARE_SERVE, cpp_opts) < 0) return 1;
  if (create_dirs(PREPARE_SERVE) < 0) return 1;
  if (teamdb_open(global->teamdb_file, global->passwd_file, 0) < 0) return 1;
  if (run_open(global->run_log_file, 0) < 0) return 1;
  if (clar_open(global->clar_log_file, 0) < 0) return 1;
  if (write_submit_templates(global->status_dir) < 0) return 1;
  if (do_loop() < 0) return 1;

  return 0;

 print_usage:
  printf(_("Usage: %s [ OPTS ] config-file\n"), argv[0]);
  printf(_("  -E     - enable C preprocessor\n"));
  printf(_("  -DDEF  - define a symbol for preprocessor\n"));
  return code;
}

/**
 * Local variables:
 *  compile-command: "make"
 *  c-font-lock-extra-types: ("\\sw+_t" "FILE")
 *  eval: (set-language-environment "Cyrillic-KOI8")
 *  enable-multibute-characters: nil
 * End:
 */

