/* Copyright (c) 2010, Oracle and/or its affiliates

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <tap.h>
#include <my_global.h>
#include <my_sys.h>


/*
  Test that like_range() returns well-formed results.
*/
static int
test_like_range_for_charset(CHARSET_INFO *cs, const char *src, size_t src_len)
{
  char min_str[32], max_str[32];
  size_t min_len, max_len, min_well_formed_len, max_well_formed_len;
  int error= 0;
  
  cs->coll->like_range(cs, src, src_len, '\\', '_', '%',
                       sizeof(min_str),  min_str, max_str, &min_len, &max_len);
  diag("min_len=%d\tmax_len=%d\t%s", (int) min_len, (int) max_len, cs->name);
  min_well_formed_len= cs->cset->well_formed_len(cs,
                                                 min_str, min_str + min_len,
                                                 10000, &error);
  max_well_formed_len= cs->cset->well_formed_len(cs,
                                                 max_str, max_str + max_len,
                                                 10000, &error);
  if (min_len != min_well_formed_len)
    diag("Bad min_str: min_well_formed_len=%d min_str[%d]=0x%02X",
          (int) min_well_formed_len, (int) min_well_formed_len,
          (uchar) min_str[min_well_formed_len]);
  if (max_len != max_well_formed_len)
    diag("Bad max_str: max_well_formed_len=%d max_str[%d]=0x%02X",
          (int) max_well_formed_len, (int) max_well_formed_len,
          (uchar) max_str[max_well_formed_len]);
  return
    min_len == min_well_formed_len &&
    max_len == max_well_formed_len ? 0 : 1;
}


static CHARSET_INFO *charset_list[]=
{
#ifdef HAVE_CHARSET_big5
  &my_charset_big5_chinese_ci,
  &my_charset_big5_bin,
#endif
#ifdef HAVE_CHARSET_euckr
  &my_charset_euckr_korean_ci,
  &my_charset_euckr_bin,
#endif
#ifdef HAVE_CHARSET_gb2312
  &my_charset_gb2312_chinese_ci,
  &my_charset_gb2312_bin,
#endif
#ifdef HAVE_CHARSET_gbk
  &my_charset_gbk_chinese_ci,
  &my_charset_gbk_bin,
#endif
#ifdef HAVE_CHARSET_latin1
  &my_charset_latin1,
  &my_charset_latin1_bin,
#endif
#ifdef HAVE_CHARSET_sjis
  &my_charset_sjis_japanese_ci,
  &my_charset_sjis_bin,
#endif
#ifdef HAVE_CHARSET_tis620
  &my_charset_tis620_thai_ci,
  &my_charset_tis620_bin,
#endif
#ifdef HAVE_CHARSET_ujis
  &my_charset_ujis_japanese_ci,
  &my_charset_ujis_bin,
#endif
#ifdef HAVE_CHARSET_utf8
  &my_charset_utf8_general_ci,
#ifdef HAVE_UCA_COLLATIONS
  &my_charset_utf8_unicode_ci,
#endif
  &my_charset_utf8_bin,
#endif
};


typedef struct
{
  const char *a;
  size_t alen;
  const char *b;
  size_t blen;
  int res;
} STRNNCOLL_PARAM;


#define CSTR(x)  (x),(sizeof(x)-1)

/*
  Byte sequence types used in the tests:
    8BIT     - a 8 bit byte (>=00x80) which makes a single byte characters
    MB2      - two bytes that make a valid character
    H2       - a byte which is a valid MB2 head byte
    T2       - a byte which is a valid MB2 tail byte
    ILSEQ    - a byte which makes an illegal sequence
    H2+ILSEQ - a sequence that starts with a valid H2 byte,
               but not followed by a valid T2 byte.

  Charset H2               T2                      8BIT
  ------- ---------------- ---------------         -------- 
  big5    [A1..F9]         [40..7E,A1..FE]
  euckr   [81..FE]         [41..5A,61..7A,81..FE]
  gb2312  [A1..F7]         [A1..FE]
  gbk     [81..FE]         [40..7E,80..FE]

  cp932   [81..9F,E0..FC]  [40..7E,80..FC]         [A1..DF]
  sjis    [81..9F,E0..FC]  [40..7E,80..FC]         [A1..DF]


  Essential byte sequences in various character sets:

  Sequence  big5   cp932      euckr  gb2312    gbk   sjis
  --------  ----   -----      -----  ------    ---   ----
  80        ILSEQ  ILSEQ      ILSEQ  ILSEQ     ILSEQ ILSEQ
  81        ILSEQ  H2         H2     ILSEQ     H2    H2
  A1        H2     8BIT       H2     H2        H2    8BIT
  A1A1      MB2    8BIT+8BIT  MB2    MB2       MB2   8BIT+8BIT
  E0E0      MB2    MB2        MB2    MB2       MB2   MB2
  F9FE      MB2    H2+ILSEQ   MB2    ILSEQ+T2  MB2   H2+ILSEQ
*/


/*
  For character sets that have the following byte sequences:
    80   - ILSEQ
    81   - ILSEQ or H2
    F9   - ILSEQ or H2
    A1A1 - MB2 or 8BIT+8BIT
    E0E0 - MB2
*/
STRNNCOLL_PARAM strcoll_mb2_common[]=
{
  /* Compare two good sequences */
  {CSTR(""),         CSTR(""),           0},
  {CSTR(""),         CSTR(" "),          0},
  {CSTR(""),         CSTR("A"),         -1},
  {CSTR(""),         CSTR("a"),         -1},
  {CSTR(""),         CSTR("\xA1\xA1"),  -1},
  {CSTR(""),         CSTR("\xE0\xE0"),  -1},

  {CSTR(" "),        CSTR(""),          0},
  {CSTR(" "),        CSTR(" "),         0},
  {CSTR(" "),        CSTR("A"),        -1},
  {CSTR(" "),        CSTR("a"),        -1},
  {CSTR(" "),        CSTR("\xA1\xA1"), -1},
  {CSTR(" "),        CSTR("\xE0\xE0"), -1},

  {CSTR("a"),        CSTR(""),          1},
  {CSTR("a"),        CSTR(" "),         1},
  {CSTR("a"),        CSTR("a"),         0},
  {CSTR("a"),        CSTR("\xA1\xA1"), -1},
  {CSTR("a"),        CSTR("\xE0\xE0"), -1},

  {CSTR("\xA1\xA1"), CSTR("\xA1\xA1"),  0},
  {CSTR("\xA1\xA1"), CSTR("\xE0\xE0"), -1},

  /* Compare a good character to an illegal or an incomplete sequence */
  {CSTR(""),         CSTR("\x80"),     -1},
  {CSTR(""),         CSTR("\x81"),     -1},
  {CSTR(""),         CSTR("\xF9"),     -1},

  {CSTR(" "),        CSTR("\x80"),     -1},
  {CSTR(" "),        CSTR("\x81"),     -1},
  {CSTR(" "),        CSTR("\xF9"),     -1},

  {CSTR("a"),        CSTR("\x80"),     -1},
  {CSTR("a"),        CSTR("\x81"),     -1},
  {CSTR("a"),        CSTR("\xF9"),     -1},

  {CSTR("\xA1\xA1"), CSTR("\x80"),     -1},
  {CSTR("\xA1\xA1"), CSTR("\x81"),     -1},
  {CSTR("\xA1\xA1"), CSTR("\xF9"),     -1},

  {CSTR("\xE0\xE0"), CSTR("\x80"),     -1},
  {CSTR("\xE0\xE0"), CSTR("\x81"),     -1},
  {CSTR("\xE0\xE0"), CSTR("\xF9"),     -1},

  /* Compare two bad/incomplete sequences */
  {CSTR("\x80"),     CSTR("\x80"),      0},
  {CSTR("\x80"),     CSTR("\x81"),     -1},
  {CSTR("\x80"),     CSTR("\xF9"),     -1},
  {CSTR("\x81"),     CSTR("\x81"),      0},
  {CSTR("\x81"),     CSTR("\xF9"),     -1},

  {NULL, 0, NULL, 0, 0}
};


/*
  For character sets that have good mb2 characters A1A1 and F9FE
*/
STRNNCOLL_PARAM strcoll_mb2_A1A1_mb2_F9FE[]=
{
  /* Compare two good characters */
  {CSTR(""),         CSTR("\xF9\xFE"), -1},
  {CSTR(" "),        CSTR("\xF9\xFE"), -1},
  {CSTR("a")       , CSTR("\xF9\xFE"), -1},
  {CSTR("\xA1\xA1"), CSTR("\xF9\xFE"), -1},
  {CSTR("\xF9\xFE"), CSTR("\xF9\xFE"),  0},

  /* Compare a good character to an illegal or an incomplete sequence */
  {CSTR(""),         CSTR("\xA1"),     -1},
  {CSTR(""),         CSTR("\xF9"),     -1},
  {CSTR("a"),        CSTR("\xA1"),     -1},
  {CSTR("a"),        CSTR("\xF9"),     -1},

  {CSTR("\xA1\xA1"), CSTR("\xA1"),     -1},
  {CSTR("\xA1\xA1"), CSTR("\xF9"),     -1},

  {CSTR("\xF9\xFE"), CSTR("\x80"),     -1},
  {CSTR("\xF9\xFE"), CSTR("\x81"),     -1},
  {CSTR("\xF9\xFE"), CSTR("\xA1"),     -1},
  {CSTR("\xF9\xFE"), CSTR("\xF9"),     -1},

  /* Compare two bad/incomplete sequences */
  {CSTR("\x80"),     CSTR("\xA1"),     -1},
  {CSTR("\x80"),     CSTR("\xF9"),     -1},

  {NULL, 0, NULL, 0, 0}
};


/*
  For character sets that have:
    A1A1 - a good mb2 character
    F9FE - a bad sequence
*/
STRNNCOLL_PARAM strcoll_mb2_A1A1_bad_F9FE[]=
{
  /* Compare a good character to an illegal or an incomplete sequence */
  {CSTR(""),         CSTR("\xF9\xFE"), -1},
  {CSTR(" "),        CSTR("\xF9\xFE"), -1},
  {CSTR("a")       , CSTR("\xF9\xFE"), -1},
  {CSTR("\xA1\xA1"), CSTR("\xF9\xFE"), -1},

  {CSTR(""),         CSTR("\xA1"),     -1},
  {CSTR(""),         CSTR("\xF9"),     -1},
  {CSTR("a"),        CSTR("\xA1"),     -1},
  {CSTR("a"),        CSTR("\xF9"),     -1},

  {CSTR("\xA1\xA1"), CSTR("\xA1"),     -1},
  {CSTR("\xA1\xA1"), CSTR("\xF9"),     -1},

  /* Compare two bad/incomplete sequences */
  {CSTR("\xF9\xFE"), CSTR("\x80"),     1},
  {CSTR("\xF9\xFE"), CSTR("\x81"),     1},
  {CSTR("\xF9\xFE"), CSTR("\xA1"),     1},
  {CSTR("\xF9\xFE"), CSTR("\xF9"),     1},
  {CSTR("\x80"),     CSTR("\xA1"),     -1},
  {CSTR("\x80"),     CSTR("\xF9"),     -1},
  {CSTR("\xF9\xFE"), CSTR("\xF9\xFE"),  0},

  {NULL, 0, NULL, 0, 0}
};


/*
  For character sets that have:
    80   - ILSEQ or H2
    81   - ILSEQ or H2
    A1   - 8BIT
    F9   - ILSEQ or H2
    F9FE - a bad sequence (ILSEQ+XX or H2+ILSEQ)
*/
STRNNCOLL_PARAM strcoll_mb1_A1_bad_F9FE[]=
{
  /* Compare two good characters */
  {CSTR(""),         CSTR("\xA1"),     -1},
  {CSTR("\xA1\xA1"), CSTR("\xA1"),      1},

  /* Compare a good character to an illegal or an incomplete sequence */
  {CSTR(""),         CSTR("\xF9"),     -1},
  {CSTR(""),         CSTR("\xF9\xFE"), -1},
  {CSTR(" "),        CSTR("\xF9\xFE"), -1},
  {CSTR("a"),        CSTR("\xF9\xFE"), -1},
  {CSTR("a"),        CSTR("\xA1"),     -1},
  {CSTR("a"),        CSTR("\xF9"),     -1},

  {CSTR("\xA1\xA1"), CSTR("\xF9"),     -1},
  {CSTR("\xA1\xA1"), CSTR("\xF9\xFE"), -1},

  {CSTR("\xF9\xFE"), CSTR("\x80"),     1},
  {CSTR("\xF9\xFE"), CSTR("\x81"),     1},
  {CSTR("\xF9\xFE"), CSTR("\xA1"),     1},
  {CSTR("\xF9\xFE"), CSTR("\xF9"),     1},

  {CSTR("\x80"),     CSTR("\xA1"),      1},

  /* Compare two bad/incomplete sequences */
  {CSTR("\x80"),     CSTR("\xF9"),     -1},
  {CSTR("\xF9\xFE"), CSTR("\xF9\xFE"),  0},

  {NULL, 0, NULL, 0, 0}
};


/*
  For character sets (e.g. cp932 and sjis) that have:
    8181 - a valid MB2 character
    A1   - a valid 8BIT character
    E0E0 - a valid MB2 character
  and sort in this order:
    8181 < A1 < E0E0
*/
STRNNCOLL_PARAM strcoll_8181_A1_E0E0[]=
{
  {CSTR("\x81\x81"), CSTR("\xA1"),     -1},
  {CSTR("\x81\x81"), CSTR("\xE0\xE0"), -1},
  {CSTR("\xA1"),     CSTR("\xE0\xE0"), -1},

  {NULL, 0, NULL, 0, 0}
};


static void
str2hex(char *dst, size_t dstlen, const char *src, size_t srclen)
{
  char *dstend= dst + dstlen;
  const char *srcend= src + srclen;
  for (*dst= '\0' ; dst + 3 < dstend && src < srcend; )
  {
    sprintf(dst, "%02X", (unsigned char) src[0]);
    dst+=2;
    src++;
  }
}


/*
  Check if the two comparison result are semantically equal:
  both are negative, both are positive, or both are zero.
*/
static int
eqres(int ares, int bres)
{
  return (ares < 0 && bres < 0) ||
         (ares > 0 && bres > 0) ||
         (ares == 0 && bres == 0);
}


static int
strcollsp(CHARSET_INFO *cs, const STRNNCOLL_PARAM *param)
{
  int failed= 0;
  const STRNNCOLL_PARAM *p;
  diag("%-20s %-10s %-10s %10s %10s", "Collation", "a", "b", "ExpectSign", "Actual");
  for (p= param; p->a; p++)
  {
    char ahex[64], bhex[64];
    int res= cs->coll->strnncollsp(cs, (uchar *) p->a, p->alen,
                                       (uchar *) p->b, p->blen, 0);
    str2hex(ahex, sizeof(ahex), p->a, p->alen);
    str2hex(bhex, sizeof(bhex), p->b, p->blen);
    diag("%-20s %-10s %-10s %10d %10d%s",
         cs->name, ahex, bhex, p->res, res,
         eqres(res, p->res) ? "" : " FAILED");
    if (!eqres(res, p->res))
    {
      failed++;
    }
    else
    {
      /* Test in reverse order */
      res= cs->coll->strnncollsp(cs, (uchar *) p->b, p->blen,
                                     (uchar *) p->a, p->alen, 0);
      if (!eqres(res, -p->res))
      {
        diag("Comparison in reverse order failed. Expected %d, got %d",
             -p->res, res);
        failed++;
      }
    }
  }
  return failed;
}


static int
test_strcollsp()
{
  int failed= 0;
#ifdef HAVE_CHARSET_big5
  failed+= strcollsp(&my_charset_big5_chinese_ci, strcoll_mb2_common);
  failed+= strcollsp(&my_charset_big5_chinese_ci, strcoll_mb2_A1A1_mb2_F9FE);
  failed+= strcollsp(&my_charset_big5_bin,        strcoll_mb2_common);
  failed+= strcollsp(&my_charset_big5_bin,        strcoll_mb2_A1A1_mb2_F9FE);
#endif
#ifdef HAVE_CHARSET_cp932
  failed+= strcollsp(&my_charset_cp932_japanese_ci, strcoll_mb2_common);
  failed+= strcollsp(&my_charset_cp932_japanese_ci, strcoll_mb1_A1_bad_F9FE);
  failed+= strcollsp(&my_charset_cp932_bin,         strcoll_mb2_common);
  failed+= strcollsp(&my_charset_cp932_bin,         strcoll_mb1_A1_bad_F9FE);
  failed+= strcollsp(&my_charset_cp932_japanese_ci, strcoll_8181_A1_E0E0);
  failed+= strcollsp(&my_charset_cp932_bin,         strcoll_8181_A1_E0E0);
#endif
#ifdef HAVE_CHARSET_euckr
  failed+= strcollsp(&my_charset_euckr_korean_ci, strcoll_mb2_common);
  failed+= strcollsp(&my_charset_euckr_korean_ci, strcoll_mb2_A1A1_mb2_F9FE);
  failed+= strcollsp(&my_charset_euckr_bin,       strcoll_mb2_common);
  failed+= strcollsp(&my_charset_euckr_bin,       strcoll_mb2_A1A1_mb2_F9FE);
#endif
#ifdef HAVE_CHARSET_gb2312
  failed+= strcollsp(&my_charset_gb2312_chinese_ci, strcoll_mb2_common);
  failed+= strcollsp(&my_charset_gb2312_chinese_ci, strcoll_mb2_A1A1_bad_F9FE);
  failed+= strcollsp(&my_charset_gb2312_bin,        strcoll_mb2_common);
  failed+= strcollsp(&my_charset_gb2312_bin,        strcoll_mb2_A1A1_bad_F9FE);
#endif
#ifdef HAVE_CHARSET_gbk
  failed+= strcollsp(&my_charset_gbk_chinese_ci, strcoll_mb2_common);
  failed+= strcollsp(&my_charset_gbk_chinese_ci, strcoll_mb2_A1A1_mb2_F9FE);
  failed+= strcollsp(&my_charset_gbk_bin,        strcoll_mb2_common);
  failed+= strcollsp(&my_charset_gbk_bin,        strcoll_mb2_A1A1_mb2_F9FE);
#endif
#ifdef HAVE_CHARSET_sjis
  failed+= strcollsp(&my_charset_sjis_japanese_ci, strcoll_mb2_common);
  failed+= strcollsp(&my_charset_sjis_bin,         strcoll_mb2_common);
  failed+= strcollsp(&my_charset_sjis_japanese_ci, strcoll_mb1_A1_bad_F9FE);
  failed+= strcollsp(&my_charset_sjis_bin,         strcoll_mb1_A1_bad_F9FE);
  failed+= strcollsp(&my_charset_sjis_japanese_ci, strcoll_8181_A1_E0E0);
  failed+= strcollsp(&my_charset_sjis_bin,         strcoll_8181_A1_E0E0);
#endif
  return failed;
}


int main()
{
  size_t i, failed= 0;
  
  plan(2);
  diag("Testing my_like_range_xxx() functions");
  
  for (i= 0; i < array_elements(charset_list); i++)
  {
    CHARSET_INFO *cs= charset_list[i];
    if (test_like_range_for_charset(cs, "abc%", 4))
    {
      ++failed;
      diag("Failed for %s", cs->name);
    }
  }
  ok(failed == 0, "Testing my_like_range_xxx() functions");

  diag("Testing cs->coll->strnncollsp()");
  failed= test_strcollsp();
  ok(failed == 0, "Testing cs->coll->strnncollsp()");

  return exit_status();
}
