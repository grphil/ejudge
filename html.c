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

#include "html.h"
#include "misctext.h"
#include "pathutl.h"
#include "fileutl.h"
#include "runlog.h"
#include "clarlog.h"
#include "logger.h"
#include "teamdb.h"
#include "prepare.h"
#include "base64.h"
#include "xalloc.h"
#include "osdeps.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <time.h>
#include <unistd.h>

#if CONF_HAS_LIBINTL - 0 == 1
#include <libintl.h>
#define _(x) gettext(x)
#else
#define _(x) x
#endif

FILE *
sf_fopen(char const *path, char const *flags)
{
  FILE *f = fopen(path, flags);
  if (f) return f;
  err(_("fopen(\"%s\",\"%s\") failed: %s"), path, flags, os_ErrorMsg());
  return NULL;
}

void
write_clar_view(int id, char const *clar_dir,
                const char *dir, const char *name, int mode)
{
  FILE   *f = 0;
  path_t  path;

  unsigned long  time;
  unsigned long  size;
  unsigned long  start;
  char           ip[CLAR_MAX_IP_LEN + 16];
  char           subj[CLAR_MAX_SUBJ_LEN + 16];
  char           rsubj[CLAR_MAX_SUBJ_LEN + 16];
  char          *hsubj = 0;
  int            hsubj_len = 0;
  int            from;
  int            to;
  int            flags;
  char           durstr[64];

  char *full_txt = 0;
  char *full_txt_armored;
  int   ft_len = 0, fta_len = 0;
  char  full_name_buf[64];

  pathmake(path, dir, "/", name, NULL);
  info(_("writing clar view %d to %s"), id, path);
  if (!(f = sf_fopen(path, "w"))) return;

  start = run_get_start_time();
  clar_get_record(id, &time, &size, ip, &from, &to, &flags, subj);
  hsubj_len = base64_decode(subj, strlen(subj), rsubj, 0);
  rsubj[hsubj_len] = 0;
  hsubj_len = html_armored_strlen(rsubj);
  hsubj_len = (hsubj_len + 7) & ~3;
  hsubj = alloca(hsubj_len);
  html_armor_string(rsubj, hsubj);
  duration_str(time-start, durstr, 0);

  fprintf(f, "<h2>%s #%d</h2>\n", _("Message"), id);
  fprintf(f, "<table border=\"0\">\n");
  fprintf(f, "<tr><td>%s:</td><td>%d</td></tr>\n", _("Number"), id);
  fprintf(f, "<tr><td>%s:</td><td>%s</td></tr>\n", _("Flags"),
          clar_flags_html(flags, from, to, 0, 0));
  fprintf(f, "<tr><td>%s:</td><td>%s</td></tr>\n", _("Time"),durstr);
  fprintf(f, "<tr><td>%s:</td><td>%s</td></tr>\n", _("IP address"), ip);
  fprintf(f, "<tr><td>%s:</td><td>%lu</td></tr>\n", _("Size"), size);
  fprintf(f, "<tr><td>%s:</td>", _("Sender"));
  if (!from) {
    fprintf(f, "<td><b>%s</b></td>", _("judges"));
  } else {
    fprintf(f, "<td>%s</td>", teamdb_get_login(from));
  }
  fprintf(f, "</tr>\n<tr><td>%s:</td>", _("To"));
  if (!to && !from) {
    fprintf(f, "<td><b>%s</b></td>", _("all"));
  } else if (!to) {
    fprintf(f, "<td><b>%s</b></td>", _("judges"));
  } else {
    fprintf(f, "<td>%s</td>", teamdb_get_login(to));
  }
  fprintf(f, "</tr>\n");
  fprintf(f, "<tr><td>%s:</td><td>%s</td></tr>", _("Subject"), hsubj);
  fprintf(f, "</table>\n");

  sprintf(full_name_buf, "%06d", id);
  if (generic_read_file(&full_txt, 0, &ft_len, 0,
                        clar_dir, full_name_buf, "") < 0) {
    fprintf(f, "<hr><big>%s</big><hr>\n", _("Cannot read message body"));
  } else {
    fta_len = html_armored_memlen(full_txt, ft_len);
    full_txt_armored = alloca(fta_len + 16);
    fta_len = html_armor_text(full_txt, ft_len, full_txt_armored);
    full_txt_armored[fta_len] = 0;
    fprintf(f, "<hr><pre>");
    fprintf(f, "%s", full_txt_armored);
    fprintf(f, "</pre><hr>");
  }

  fclose(f);
}

void
write_team_statistics(int team, int all_runs_flag, int all_clars_flag,
                      char const *dir, char const *name)
{
  path_t  path;
  FILE   *f = 0;
  int     i, n;
  int     showed;

  unsigned long time;
  unsigned long size;
  int           team_id;
  int           lang_id;
  int           prob_id;
  int           status;
  int           test;
  int           from;
  int           to;
  int           flags;

  char           dur_str[64];
  char           stat_str[64];
  char          *prob_str;
  char          *lang_str;

  char           subj[CLAR_MAX_SUBJ_LEN + 4];      /* base64 subj */
  char           psubj[CLAR_MAX_SUBJ_TXT_LEN + 4]; /* plain text subj */
  char          *asubj = 0; /* html armored subj */
  int            asubj_len = 0; /* html armored subj len */

  unsigned long start_time = run_get_start_time();

  int runs_to_show = all_runs_flag?100000:15;
  int clars_to_show = all_clars_flag?100000:15;

  pathmake(path, dir, "/", name, 0);
  info(_("team %d statistics to %s"), team, path);
  if (!(f = sf_fopen(path, "w"))) return;

  /* write run statistics: show last 15 in the reverse order */
  fprintf(f,"<table border=\"1\"><tr><th>%s</th><th>%s</th>"
          "<th>%s</th>"
          "<th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th></tr>\n",
          _("Run ID"), _("Time"), _("Size"), _("Problem"),
          _("Language"), _("Result"), _("Failed test"));

  for (showed = 0, i = run_get_total() - 1;
       i >= 0 && showed < runs_to_show;
       i--) {
    run_get_record(i, &time, &size, 0,
                   &team_id, &lang_id, &prob_id, &status, &test);
    if (team_id != team) continue;
    showed++;

    if (!start_time) time = start_time;
    if (start_time > time) time = start_time;
    duration_str(time - start_time, dur_str, 0);
    run_status_str(status, stat_str, 0);
    prob_str = "???";
    if (probs[prob_id]) prob_str = probs[prob_id]->short_name;
    lang_str = "???";
    if (langs[lang_id]) lang_str = langs[lang_id]->short_name;

    fputs("<tr>\n", f);
    fprintf(f, "<td>%d</td>", i);
    fprintf(f, "<td>%s</td>", dur_str);
    fprintf(f, "<td>%lu</td>", size);
    fprintf(f, "<td>%s</td>", prob_str);
    fprintf(f, "<td>%s</td>", lang_str);
    fprintf(f, "<td>%s</td>", stat_str);
    if (test <= 0) fprintf(f, "<td>%s</td>", _("N/A"));
    else fprintf(f, "<td>%d</td>", test);
    fputs("\n</tr>\n", f);
  }
  fputs("</table>\n", f);

  /* separator */
  putc(1, f);

  /* write clars statistics for the last 15 in the reverse order */
  fprintf(f,"<table border=\"1\"><tr><th>%s</th><th>%s</th><th>%s</th>"
          "<th>%s</th>"
          "<th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th></tr>\n",
          _("Clar ID"), _("Flags"), _("Time"), _("Size"), _("From"),
          _("To"), _("Subject"), _("View"));
  for (showed = 0, i = clar_get_total() - 1;
       showed < clars_to_show && i >= 0;
       i--) {
    if (clar_get_record(i, &time, &size, 0, &from, &to, &flags, subj) < 0)
      continue;
    if (from > 0 && from != team) continue;
    if (to > 0 && to != team) continue;
    showed++;

    base64_decode_str(subj, psubj, 0);
    n = html_armored_strlen(psubj);
    if (n + 4 > asubj_len) {
      asubj_len = (n + 7) & ~3;
      asubj = alloca(asubj_len);
    }
    html_armor_string(psubj, asubj);
    if (!start_time) time = start_time;
    if (start_time > time) time = start_time;
    duration_str(time - start_time, dur_str, 0);

    fputs("$1<tr>", f);
    fprintf(f, "<td>%d</td>", i);
    fprintf(f, "<td>%s</td>", clar_flags_html(flags, from, to, 0, 0));
    fprintf(f, "<td>%s</td>", dur_str);
    fprintf(f, "<td>%lu</td>", size);
    if (!from) {
      fprintf(f, "<td><b>%s</b></td>", _("judges"));
    } else {
      fprintf(f, "<td>%s</td>", teamdb_get_login(from));
    }
    if (!to && !from) {
      fprintf(f, "<td><b>%s</b></td>", _("all"));
    } else if (!to) {
      fprintf(f, "<td><b>%s</b></td>", _("judges"));
    } else {
      fprintf(f, "<td>%s</td>", teamdb_get_login(to));
    }
    fprintf(f, "<td>%s</td>", asubj);
    fprintf(f, "<td><input type=\"image\" name=\"clar_%d\" value=\"%s\" alt=\"view\" src=\"/icons/view_btn.gif\"></td>\n", i, _("view"));

    fputs("</tr></form>\n", f);
  }
  fputs("</table>\n", f);

  fclose(f);
}

void
write_team_clar(int team_id, int clar_id,
                char const *clar_dir, char const *dir, char const *name)
{
  path_t  path;
  FILE   *f;
  char    cname[64];
  char   *csrc = 0;
  int     csize = 0;

  unsigned long start_time;
  unsigned long time;
  unsigned long size;
  int from;
  int to;
  char subj[CLAR_MAX_SUBJ_LEN + 4];
  char psubj[CLAR_MAX_SUBJ_TXT_LEN + 4];
  char *asubj;
  int  asubj_len;
  char *atxt;
  int  atxt_len;
  char dur_str[64];

  pathmake(path, dir, "/", name, 0);
  if (!(f = sf_fopen(path, "w"))) return;

  start_time = run_get_start_time();
  if (clar_get_record(clar_id, &time, &size, NULL,
                      &from, &to, NULL, subj) < 0)
    goto server_failed;
  if (from > 0 && from != team_id) goto access_denied;
  if (to > 0 && to != team_id) goto access_denied;

  sprintf(cname, "%06d", clar_id);
  if (generic_read_file(&csrc, 0, &csize, 0,
                        global->clar_archive_dir, cname, "") < 0)
    goto server_failed;

  base64_decode_str(subj, psubj, 0);
  asubj_len = html_armored_strlen(psubj);
  asubj = alloca(asubj_len + 4);
  html_armor_string(psubj, asubj);
  atxt_len = html_armored_strlen(csrc);
  atxt = alloca(atxt_len + 4);
  html_armor_string(csrc, atxt);
  xfree(csrc);
  if (!start_time) time = start_time;
  if (time < start_time) time = start_time;
  duration_str(time - start_time, dur_str, 0);

  fprintf(f, "<h2>%s #%d</h2>\n", _("Message"), clar_id);
  fprintf(f, "<table border=\"0\">\n");
  fprintf(f, "<tr><td>%s:</td><td>%d</td></tr>\n", _("Number"), clar_id);
  fprintf(f, "<tr><td>%s:</td><td>%s</td></tr>\n", _("Time"), dur_str);
  fprintf(f, "<tr><td>%s:</td><td>%lu</td></tr>\n", _("Size"), size);
  fprintf(f, "<tr><td>%s:</td>", _("Sender"));
  if (!from) {
    fprintf(f, "<td><b>%s</b></td>", _("judges"));
  } else {
    fprintf(f, "<td>%s</td>", teamdb_get_name(from));
  }
  fprintf(f, "</tr>\n<tr><td>%s:</td>", _("To"));
  if (!to && !from) {
    fprintf(f, "<td><b>%s</b></td>", _("all"));
  } else if (!to) {
    fprintf(f, "<td><b>%s</b></td>", _("judges"));
  } else {
    fprintf(f, "<td>%s</td>", teamdb_get_name(to));
  }
  fprintf(f, "</tr>\n");
  fprintf(f, "<tr><td>%s:</td><td>%s</td></tr>", _("Subject"), asubj);
  fprintf(f, "</table>\n");
  fprintf(f, "<hr><pre>");
  fprintf(f, "%s", atxt);
  fprintf(f, "</pre>");

  fclose(f);
  return;

 access_denied:
  err(_("write_team_clar: access denied"));
  fprintf(f, "<h2>%s</h2><p>%s</p>", _("Access denied"),
          _("You do not have permissions to view this message."));
  fclose(f);

 server_failed:
  err(_("write_team_clar: server_failed"));
  fprintf(f, "<h2>%s</h2><p>%s</p>",
          _("Server is unable to perform your request"),
          _("Internal server error"));
  fclose(f);
  return;

}

static void
do_write_standings(FILE *f, int client_flag)
{
  int      i, j;

  int     *t_ind;
  int      t_max;
  int      t_tot;
  int     *t_prob;
  int     *t_pen;
  int     *t_rev;
  int     *t_sort;
  int     *t_n1;
  int     *t_n2;
  int     *p_ind;
  int     *p_rev;
  int      p_max;
  int      p_tot;
  int    **calc;
  int      r_tot, k;
  int      tt, pp;

  unsigned long **ok_time;

  char     dur_str[64];

  unsigned long start_time;
  unsigned long stop_time;
  unsigned long cur_time;
  unsigned long run_time;
  int           team_id;
  int           prob_id;
  int           status;

  char          header[1024];

  start_time = run_get_start_time();
  stop_time = run_get_stop_time();
  if (!start_time) {
    if (global->name[0]) {
      sprintf(header, "%s &quot;%s&quot; - %s",
              _("Contest"), _("team standings"),
              global->name);
    } else {
      sprintf(header, "%s", _("Team standings"));
    }

    if (!client_flag) {
      fprintf(f, "<html><head><meta http-equiv=\"Content-Type\" content=\"%s\"><title>%s</title></head><body><h1>%s</h1>\n", _("text/html"),
              header, header);
    } else {
      fprintf(f, "%s%c", header, 1);
    }
    fprintf(f, "<h2>%s</h2>", _("The contest is not started"));
    if (!client_flag) {
      fprintf(f, "</body></html>");
    }
    return;
  }

  /* make team index */
  t_max = teamdb_get_max_team_id() + 1;
  if (!XALLOCA(t_ind, t_max)) goto alloca_failed;
  XMEMZERO(t_ind, t_max);
  if (!XALLOCA(t_rev, t_max)) goto alloca_failed;
  XMEMZERO(t_rev, t_max);
  for (i = 1, t_tot = 0; i < t_max; i++) {
    t_rev[i] = -1;
    if (!teamdb_lookup(i)) continue;
    if ((teamdb_get_flags(i) & (TEAM_INVISIBLE | TEAM_BANNED))) continue;
    t_rev[i] = t_tot;
    t_ind[t_tot++] = i;
  }
  if (!XALLOCA(t_prob, t_tot)) goto alloca_failed;
  XMEMZERO(t_prob, t_tot);
  if (!XALLOCA(t_pen,t_tot)) goto alloca_failed;
  XMEMZERO(t_pen, t_tot);
  if (!XALLOCA(t_sort, t_tot)) goto alloca_failed;
  for (i = 0; i < t_tot; i++)
    t_sort[i] = i;
  if (!XALLOCA(t_n1, t_tot)) goto alloca_failed;
  if (!XALLOCA(t_n2, t_tot)) goto alloca_failed;

  /* make problem index */
  p_max = max_prob + 1;
  if (!XALLOCA(p_ind, p_max)) goto alloca_failed;
  XMEMZERO(p_ind, p_max);
  if (!XALLOCA(p_rev, p_max)) goto alloca_failed;
  XMEMZERO(p_rev, p_max);
  for (i = 1, p_tot = 0; i < p_max; i++) {
    p_rev[i] = -1;
    if (!probs[i]) continue;
    p_rev[i] = p_tot;
    p_ind[p_tot++] = i;
  }

  /* make calculation table */
  if (!XALLOCA(calc, t_tot)) goto alloca_failed;
  XMEMZERO(calc, t_tot);
  if (!XALLOCA(ok_time, t_tot)) goto alloca_failed;
  for (i = 0; i < t_tot; i++) {
    if (!XALLOCA(calc[i], p_tot)) goto alloca_failed;
    XMEMZERO(calc[i], p_tot);
    if (!XALLOCA(ok_time[i], p_tot)) goto alloca_failed;
    XMEMZERO(ok_time[i], p_tot);
  }

  /* now scan runs log */
  r_tot = run_get_total();
  for (k = 0; k < r_tot; k++) {
    run_get_record(k, &run_time, 0, 0, &team_id, 0, &prob_id, &status, 0);
    if (team_id <= 0 || team_id >= t_max) continue;
    if (t_rev[team_id] < 0) continue;
    if (prob_id <= 0 || prob_id > max_prob) continue;
    if (p_rev[prob_id] < 0) continue;
    tt = t_rev[team_id];
    pp = p_rev[prob_id];

    if (status == 0) {
      /* program accepted */
      if (calc[tt][pp] > 0) continue;

      t_pen[tt] += 20 * -calc[tt][pp];
      calc[tt][pp] = 1 - calc[tt][pp];
      t_prob[tt]++;
      if (run_time < start_time) run_time = start_time;
      ok_time[tt][pp] = (run_time - start_time + 59) / 60;
      t_pen[tt] += ok_time[tt][pp];
    } else if (status > 0 && status < 6) {
      /* some error */
      if (calc[tt][pp] <= 0) calc[tt][pp]--;
    }
  }

  /* now sort the teams in the descending order */
  for (i = 0; i < t_tot - 1; i++) {
    int maxind = i, temp;
    for (j = i + 1; j < t_tot; j++) {
      if (t_prob[t_sort[j]] > t_prob[t_sort[maxind]]
          || (t_prob[t_sort[j]] == t_prob[t_sort[maxind]] && t_pen[t_sort[j]] < t_pen[t_sort[maxind]]))
        maxind = j;
    }
    temp = t_sort[i];
    t_sort[i] = t_sort[maxind];
    t_sort[maxind] = temp;
  }

  /* now resolve ties */
  for(i = 0; i < t_tot;) {
    for (j = i + 1; j < t_tot; j++) {
      if (t_prob[t_sort[i]] != t_prob[t_sort[j]]
          || t_pen[t_sort[i]] != t_pen[t_sort[j]]) break;
    }
    for (k = i; k < j; k++) {
      t_n1[k] = i;
      t_n2[k] = j - 1;
    }
    i = j;
  }

  /* now print this stuff */
  cur_time = time(0);
  if (start_time > cur_time) cur_time = start_time;
  if (stop_time && cur_time > stop_time) cur_time = stop_time;
  duration_str(cur_time - start_time, dur_str, 0);

  if (global->name[0]) {
    sprintf(header, "%s &quot;%s&quot; - %s [%s]",
            _("Contest"), _("team standings"),
            global->name, dur_str);
  } else {
    sprintf(header, "%s [%s]", _("Team standings"), dur_str);
  }

  if (!client_flag) {
    fprintf(f, "<html><head><meta http-equiv=\"Content-Type\" content=\"%s\"><title>%s</title></head><body><h1>%s</h1>",
            _("text/html"),
            header, header);
  } else {
    fprintf(f, "%s%c", header, 1);
  }

  /* print table header */
  fprintf(f, "<table border=\"1\"><tr><th>%s</th><th>%s</th>",
          _("Place"), _("Team"));
  for (j = 0; j < p_tot; j++) {
    fprintf(f, "<th>%s</th>", probs[p_ind[j]]->short_name);
  }
  fprintf(f, "<th>%s</th><th>%s</th></tr>",
          _("Total"), _("Penalty"));

  for (i = 0; i < t_tot; i++) {
    int t = t_sort[i];
    fputs("<tr><td>", f);
    if (t_n1[i] == t_n2[i]) fprintf(f, "%d", t_n1[i] + 1);
    else fprintf(f, "%d-%d", t_n1[i] + 1, t_n2[i] + 1);
    fputs("</td>", f);
    fprintf(f, "<td>%s</td>", teamdb_get_name(t_ind[t]));
    for (j = 0; j < p_tot; j++) {
      if (calc[t][j] < 0) {
        fprintf(f, "<td>%d</td>", calc[t][j]);
      } else if (calc[t][j] == 1) {
        fprintf(f, "<td>+ (%ld:%02ld)</td>",
                ok_time[t][j] / 60, ok_time[t][j] % 60);
      } else if (calc[t][j] > 0) {
        fprintf(f, "<td>+%d (%ld:%02ld)</td>",
                calc[t][j] - 1, ok_time[t][j] / 60, ok_time[t][j] % 60);
      } else {
        fprintf(f, "<td>&nbsp;</td>");
      }
    }
    fprintf(f, "<td>%d</td><td>%d</td></tr>",
            t_prob[t], t_pen[t]);
  }

  fputs("</table>\n", f);
  if (!client_flag) fputs("</body></html>", f);
  fclose(f);

  return;
 alloca_failed: 
  err(_("alloca failed"));
  fclose(f);
  return;
}

void
write_standings(char const *stat_dir, char const *name)
{
  char    tbuf[64];
  path_t  tpath;
  FILE   *f;

  sprintf(tbuf, "XXX_%lu%d", time(0), getpid());
  pathmake(tpath, stat_dir, "/", tbuf, 0);
  if (!(f = sf_fopen(tpath, "w"))) return;
  do_write_standings(f, 0);
  fclose(f);
  generic_copy_file(REMOVE, stat_dir, tbuf, "",
                    SAFE, stat_dir, name, "");
  return;
}

static void
write_judge_allruns(int master_mode, int show_all, FILE *f)
{
  int total;
  int show_num;
  int i;

  unsigned long time, start, size;
  int teamid, langid, probid, status, test;

  char ip[RUN_MAX_IP_LEN + 4];
  char durstr[64], statstr[64];

  start = run_get_start_time();
  total = run_get_total();
  show_num = 10;
  if (show_all) show_num = total;

  /* header */
  //fprintf(f, "<p><big>����� �������: %d</big></p>\n", total);
  fprintf(f, "<p><big>%s: %d</big></p>\n",
          _("Total submissions"), total);
  fprintf(f, "<table border=\"1\"><tr><th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th>", 
          _("Run ID"), _("Time"), _("Size"), _("IP"),
          _("Team login"), _("Team name"), _("Problem"),
          _("Language"), _("Result"), _("Failed test"));
  if (master_mode) {
    fprintf(f, "<th>%s</th><th>%s</th><th>%s</th>",
            _("New result"), _("New test"), _("Change result"));
  }
  fprintf(f, "<th>%s</th><th>%s</th></tr>\n",
          _("View source"), _("View report"));

  for (i = total - 1; i >= 0 && show_num; i--, show_num--) {
    run_get_record(i, &time, &size, ip,
                   &teamid, &langid, &probid, &status, &test);

    if (!start) time = start;
    if (start > time) time = start;
    duration_str(time - start, durstr, 0);
    run_status_str(status, statstr, 0);

    fputs("$1", f);

    fputs("<tr>", f);
    fprintf(f, "<td>%d</td>", i);
    fprintf(f, "<td>%s</td>", durstr);
    fprintf(f, "<td>%lu</td>", size);
    fprintf(f, "<td>%s</td>", ip);
    fprintf(f, "<td>%s</td>", teamdb_get_login(teamid));
    fprintf(f, "<td>%s</td>", teamdb_get_name(teamid));
    if (probs[probid]) fprintf(f, "<td>%s</td>", probs[probid]->short_name);
    else fprintf(f, "<td>??? - %d</td>", probid);
    if (langs[langid]) fprintf(f, "<td>%s</td>", langs[langid]->short_name);
    else fprintf(f, "<td>??? - %d</td>", langid);
    fprintf(f, "<td>%s</td>", statstr);
    if (test <= 0) fprintf(f, "<td>%s</td>\n", _("N/A"));
    else fprintf(f, "<td>%d</td>\n", test);

    if (master_mode) {
      fprintf(f,
              "<td><select name=\"stat_%d\">"
              "<option value=\"\"> "
              "<option value=\"0\">%s"
              "<option value=\"1\">%s"
              "<option value=\"2\">%s"
              "<option value=\"3\">%s"
              "<option value=\"4\">%s"
              "<option value=\"5\">%s"
              "<option value=\"99\">%s"
              "</select></td>\n", i,
              _("ACCEPTED"), _("Compilation"), _("Run-time"),
              _("Time-limit"), _("Presentation"), _("Wrong"), _("Rejudge"));

      fprintf(f,
              "<td><input type=\"text\" name=\"failed_%d\" size=\"2\"></td>",
              i);

      fprintf(f,
              "<td><input type=\"submit\" name=\"change_%d\""
              " value=\"%s\"></td>\n", i, _("change"));
    }

    fprintf(f, "<td><input type=\"image\" name=\"source_%d\" value=\"%s\" alt=\"view\" src=\"/icons/view_btn.gif\"></td>\n", i, _("view"));
    fprintf(f, "<td><input type=\"image\" name=\"report_%d\" value=\"%s\" alt=\"view\" src=\"/icons/view_btn.gif\"></td>\n", i, _("view"));

    fputs("</tr></form>\n", f);
  }

  fputs("</table>\n", f);
}

static void
write_judge_allclars(int master_mode, int show_all, FILE *f)
{
  int total;
  int show_num = 15;

  unsigned long start, time, size;
  int from, to, flags;
  int i;

  char subj[CLAR_MAX_SUBJ_LEN + 4];
  char psubj[CLAR_MAX_SUBJ_TXT_LEN + 4];
  char ip[CLAR_MAX_IP_LEN + 4];
  char *asubj = 0;
  int   asubj_len = 0, new_len;

  char durstr[64];

  start = run_get_start_time();
  total = clar_get_total();
  if (show_all) show_num = total;

  //fprintf(f, "<p><big>����� ���������: %d</big></p>\n", total);
  fprintf(f, "<p><big>%s: %d</big></p>\n", _("Total messages"), total);
  fprintf(f, "<table border=\"1\"><tr><th>%s</th><th>%s</th><th>%s</th>"
          "<th>%s</th>"
          "<th>%s</th>"
          "<th>%s</th><th>%s</th>"
          "<th>%s</th><th>%s</th></tr>\n",
          _("Clar ID"), _("Flags"), _("Time"), _("IP"), _("Size"),
          _("From"), _("To"), _("Subject"), _("View"));

  for (i = total - 1; i >= 0 && show_num; i--, show_num--) {
    clar_get_record(i, &time, &size, ip, &from, &to, &flags, subj);
    if (!master_mode && (from <= 0 || flags >= 2)) continue; 

    base64_decode_str(subj, psubj, 0);
    new_len = html_armored_strlen(psubj);
    new_len = (new_len + 7) & ~3;
    if (new_len > asubj_len) asubj = alloca(asubj_len = new_len);
    html_armor_string(psubj, asubj);
    if (!start) time = start;
    if (start > time) time = start;
    duration_str(time - start, durstr, 0);

    fputs("$1<tr>", f);
    fprintf(f, "<td>%d</td>", i);
    fprintf(f, "<td>%s</td>", clar_flags_html(flags, from, to, 0, 0));
    fprintf(f, "<td>%s</td>", durstr);
    fprintf(f, "<td>%s</td>", ip);
    fprintf(f, "<td>%lu</td>", size);
    if (!from) {
      fprintf(f, "<td><b>%s</b></td>", _("judges"));
    } else {
      fprintf(f, "<td>%s</td>", teamdb_get_login(from));
    }
    if (!to && !from) {
      fprintf(f, "<td><b>%s</b></td>", _("all"));
    } else if (!to) {
      fprintf(f, "<td><b>%s</b></td>", _("judges"));
    } else {
      fprintf(f, "<td>%s</td>", teamdb_get_login(to));
    }
    fprintf(f, "<td>%s</td>", asubj);
    fprintf(f, "<input type=\"hidden\" name=\"enable_reply\" value=\"%d\">",
            !!from);
    fprintf(f, "<td><input type=\"image\" name=\"clar_%d\" value=\"%s\" alt=\"view\" src=\"/icons/view_btn.gif\"></td>\n", i, _("view"));

    fputs("</tr></form>\n", f);
  }

  fputs("</table>\n", f);
}

void
write_judge_allstat(int master_mode, int all_runs, int all_clars,
                    char const *dir, char const *name)
{
  path_t  path;
  FILE   *f;

  pathmake(path, dir, "/", name, 0);
  info(_("judge statistics to %s"), path);
  if (!(f = sf_fopen(path, "w"))) return;

  write_judge_allruns(master_mode, all_runs, f);
  putc(1, f);
  write_judge_allclars(master_mode, all_clars, f);

  fclose(f);
}

void
write_judge_source_view(char const *pk_name, int rid)
{
  path_t  path;
  FILE   *f = 0;
  path_t  src_base;
  path_t  src_path;
  char   *src = 0;
  char   *html = 0;
  int     src_len = 0;
  int     html_len;

  ASSERT(rid >= 0 && rid < run_get_total());
  pathmake(path, global->pipe_dir, "/", pk_name, 0);
  if (!(f = sf_fopen(path, "w"))) return;

  sprintf(src_base, "%06d", rid);
  pathmake(src_path, global->run_archive_dir, "/", src_base, 0);
  if (generic_read_file(&src, 0, &src_len, 0, 0, src_path, "") < 0) {
    fprintf(f, "<h2>%s</h2><p>%s</p>",
            _("Server is unable to perform your request"),
            _("Source file is not found"));
    goto _cleanup;
  }

  html_len = html_armored_memlen(src, src_len);
  html = alloca(html_len + 16);
  html_armor_text(src, src_len, html);
  html[html_len] = 0;

  fprintf(f, "<pre>%s</pre>", html);

 _cleanup:
  if (f) fclose(f);
  if (src) xfree(src);  
}

void
write_judge_report_view(char const *pk_name, int rid)
{
  path_t  path;
  FILE   *f = 0;
  path_t  report_base;
  path_t  report_path;
  char   *report = 0;
  char   *html_report = 0;
  int     report_len;
  int     html_len;

  ASSERT(rid >= 0 && rid < run_get_total());
  pathmake(path, global->pipe_dir, "/", pk_name, 0);
  if (!(f = sf_fopen(path, "w"))) return;

  sprintf(report_base, "%06d", rid);
  pathmake(report_path, global->report_archive_dir, "/", report_base, 0);
  if (generic_read_file(&report, 0, &report_len, 0, 0, report_path, "") < 0) {
    fprintf(f, "<h2>%s</h2><p>%s</p>",
            _("Server is unable to perform your request"),
            _("Report file is not found"));
    goto _cleanup;
  }

  html_len = html_armored_memlen(report, report_len);
  html_report = alloca(html_len + 16);
  html_armor_text(report, report_len, html_report);
  html_report[html_len] = 0;

  fprintf(f, "<pre>%s</pre>", html_report);

 _cleanup:
  if (f) fclose(f);
  if (report) xfree(report);
}

void
write_judge_standings(char const *pk_name)
{
  path_t  path;
  FILE   *f;

  pathmake(path, global->pipe_dir, "/", pk_name, 0);
  if (!(f = sf_fopen(path, "w"))) return;
  do_write_standings(f, 1);
  fclose(f);
}

/**
 * Local variables:
 *  compile-command: "make"
 *  c-font-lock-extra-types: ("\\sw+_t" "FILE")
 *  eval: (set-language-environment "Cyrillic-KOI8")
 *  enable-multibute-characters: nil
 * End:
 */

