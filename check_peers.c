#include <check.h>
#include "peers.h"
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>


START_TEST(len1)
{
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(peerSet == NULL);
  ck_assert(size == 0);
  struct in_addr ia;
  ia.s_addr = 33;
  putip(&ia);
  //ck_assert(8 == 9);
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(size == 1);
}
END_TEST

START_TEST(update_len1)
{
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(peerSet == NULL);
  ck_assert(size == 0);
  struct in_addr ia;
  ia.s_addr = 33;
  putip(&ia);
  ck_assert(peerSet->last_seen == time(NULL));
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(size == 1);
  sleep(1.5);
  putip(&ia);
  ck_assert(peerSet->last_seen == time(NULL));
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(size == 1);
}
END_TEST

START_TEST(update_first)
{
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(peerSet == NULL);
  ck_assert(size == 0);
  struct in_addr ia1, ia2, ia3, ia4;
  ia1.s_addr = 33;
  ia2.s_addr = 44;
  ia3.s_addr = 55;
  ia4.s_addr = 55;
  putip(&ia1);
  ck_assert(peerSet->sin_addr.s_addr == 33);
  ck_assert(peerSet->last_seen == time(NULL));
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(size == 1);
  sleep(1.5);
  putip(&ia2);
  ck_assert(peerSet->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > lastPeer->last_seen);
  ck_assert(size == 2);
  sleep(1.5);
  putip(&ia3);
  ck_assert(peerSet->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > peerSet->next->last_seen);
  ck_assert(lastPeer->last_seen < peerSet->next->last_seen);
  ck_assert(peerSet->prev == NULL);
  ck_assert(lastPeer->next == NULL);
  ck_assert(size == 3);
  // updating first peer
  sleep(1.5);
  putip(&ia4);
  ck_assert(peerSet->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > peerSet->next->last_seen);
  ck_assert(lastPeer->last_seen < peerSet->next->last_seen);
  ck_assert(peerSet->prev == NULL);
  ck_assert(lastPeer->next == NULL);
  ck_assert(size == 3);
}
END_TEST
START_TEST(update_last)
{
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(peerSet == NULL);
  ck_assert(size == 0);
  struct in_addr ia1, ia2, ia3, ia4;
  ia1.s_addr = 33;
  ia2.s_addr = 44;
  ia3.s_addr = 55;
  ia4.s_addr = 33;
  putip(&ia1);
  ck_assert(peerSet->sin_addr.s_addr == 33);
  ck_assert(peerSet->last_seen == time(NULL));
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(size == 1);
  sleep(1.5);
  putip(&ia2);
  ck_assert(peerSet->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > lastPeer->last_seen);
  ck_assert(size == 2);
  sleep(1.5);
  putip(&ia3);
  ck_assert(peerSet->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > peerSet->next->last_seen);
  ck_assert(lastPeer->last_seen < peerSet->next->last_seen);
  ck_assert(peerSet->prev == NULL);
  ck_assert(lastPeer->next == NULL);
  ck_assert(size == 3);
  // updating first peer
  sleep(1.5);
  putip(&ia4);
  ck_assert(peerSet->sin_addr.s_addr == 33);
  ck_assert(peerSet->next->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 44);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > peerSet->next->last_seen);
  ck_assert(lastPeer->last_seen < peerSet->next->last_seen);
  ck_assert(peerSet->prev == NULL);
  ck_assert(lastPeer->next == NULL);
  ck_assert(size == 3);
}
END_TEST
START_TEST(update_middle)
{
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(peerSet == NULL);
  ck_assert(size == 0);
  struct in_addr ia1, ia2, ia3, ia4;
  ia1.s_addr = 33;
  ia2.s_addr = 44;
  ia3.s_addr = 55;
  ia4.s_addr = 44;
  putip(&ia1);
  ck_assert(peerSet->sin_addr.s_addr == 33);
  ck_assert(peerSet->last_seen == time(NULL));
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(size == 1);
  sleep(1.5);
  putip(&ia2);
  ck_assert(peerSet->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > lastPeer->last_seen);
  ck_assert(size == 2);
  sleep(1.5);
  putip(&ia3);
  ck_assert(peerSet->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > peerSet->next->last_seen);
  ck_assert(lastPeer->last_seen < peerSet->next->last_seen);
  ck_assert(peerSet->prev == NULL);
  ck_assert(lastPeer->next == NULL);
  ck_assert(size == 3);
  // updating first peer
  sleep(1.5);
  putip(&ia4);
  ck_assert(peerSet->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 33);
  ck_assert((long)peerSet != (long)lastPeer);
  ck_assert((long)peerSet->next->next == (long)lastPeer);
  ck_assert(peerSet->last_seen > peerSet->next->last_seen);
  ck_assert(lastPeer->last_seen < peerSet->next->last_seen);
  ck_assert(peerSet->prev == NULL);
  ck_assert(lastPeer->next == NULL);
  ck_assert(size == 3);
}
END_TEST
START_TEST(overlen)
{
  ck_assert((long)peerSet == (long)lastPeer);
  ck_assert(peerSet == NULL);
  ck_assert(size == 0);
  struct in_addr ia1, ia2, ia3, ia4, ia5, ia6;
  ia1.s_addr = 11;
  ia2.s_addr = 22;
  ia3.s_addr = 33;
  ia4.s_addr = 44;
  ia5.s_addr = 55;
  ia6.s_addr = 66;
  putip(&ia1);
  ck_assert(peerSet->sin_addr.s_addr == 11);
  ck_assert(peerSet->last_seen == time(NULL));
  ck_assert(size == 1);
  //sleep(1.0);
  putip(&ia2);
  //sleep(1.0);
  putip(&ia3);
  //sleep(1.0);
  putip(&ia4);
  //sleep(1.0);
  putip(&ia5);
  ck_assert(peerSet->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 33);
  ck_assert(peerSet->next->next->next->sin_addr.s_addr == 22);
  ck_assert(peerSet->next->next->next->next->sin_addr.s_addr == 11);
  ck_assert(size == 5);
  ////sleep(1.0);
  putip(&ia6);

  ck_assert(peerSet->sin_addr.s_addr == 66);
  ck_assert(peerSet->next->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->next->sin_addr.s_addr == 33);
  ck_assert(peerSet->next->next->next->next->sin_addr.s_addr == 22);
  ck_assert(peerSet->next->next->next->next->next == NULL);
  ck_assert(size == 5);
  sleep(1.0);
  putip(&ia2);

  ck_assert(peerSet->sin_addr.s_addr == 22);
  ck_assert(peerSet->next->sin_addr.s_addr == 66);
  ck_assert(peerSet->next->next->sin_addr.s_addr == 55);
  ck_assert(peerSet->next->next->next->sin_addr.s_addr == 44);
  ck_assert(peerSet->next->next->next->next->sin_addr.s_addr == 33);
  ck_assert(peerSet->next->next->next->next->next == NULL);
  ck_assert(size == 5);
}
END_TEST

Suite * money_suite(void)
{
  Suite *s;
  TCase *tc_core;

  s = suite_create("PeerSet");

  /* Core test case */
  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, len1);
  tcase_add_test(tc_core, update_len1);
  tcase_add_test(tc_core, update_first);
  tcase_add_test(tc_core, update_middle);
  tcase_add_test(tc_core, update_last);
  tcase_add_test(tc_core, overlen);
  suite_add_tcase(s, tc_core);

  return s;
}

int main(void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = money_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
