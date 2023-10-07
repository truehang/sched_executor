#include "preprocess.hpp"

#include <gtest/gtest.h>

#include <utility>
using namespace gstone::exec;
TEST(TriggeredDirectedGraphGroup, Test0) {
  DirectedGraph dg({"A -> E", "E -> H", "E -> I", "H -> K", "I -> L", "I -> M",
                    "M -> O", "O -> P", "O -> Q"});

  EXPECT_NE(dg.getVertex("E"), dg.getVertex("A"));
  EXPECT_EQ(dg.getVertex("E")->supplementary_information_.preVertices.size(),
            1);
  EXPECT_EQ(dg.getVertex("A")->supplementary_information_.postVertices.size(),
            1);
  auto newvec = dg.getVertex("A")->supplementary_information_.postVertices;
  EXPECT_EQ(newvec.size(), 1);

  TaskBasicInformationPool pool;
  EXPECT_EQ(pool.newTask("A", 2, 6, 0, 0), true);
  EXPECT_EQ(pool.newTask("E", 2), true);
  EXPECT_EQ(pool.newTask("H", 2), true);
  EXPECT_EQ(pool.newTask("I", 1), true);
  EXPECT_EQ(pool.newTask("K", 2), true);
  EXPECT_EQ(pool.newTask("L", 1), true);
  EXPECT_EQ(pool.newTask("M", 1), true);
  EXPECT_EQ(pool.newTask("O", 1), true);
  EXPECT_EQ(pool.newTask("P", 2), true);
  EXPECT_EQ(pool.newTask("Q", 1), true);
  EXPECT_EQ(preCheck(dg, pool), true);

  TtsDeadlineParser ttsDeadline(
      {"H : 8", "K : 12", "L : 10", "P : 11", "Q : 11"});
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);
  EXPECT_EQ(maxFrom(dg.getVertex("E"), pool), dg.getVertex("A"));

  EXPECT_EQ(tdgg.deadlinePropagation(ttsDeadline, pool).first, true);
  pool.dump();
  TaskInstancePool taskInstances;
  TaskInstanceQueue Queue;
  Cpu Cpu3core(3);
  ExecutionSegmentPool ExeSegs;
  LeastWaitingTimeBudgetFirst lwtbf;
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu3core, taskInstances, pool, tdgg, ExeSegs),
      true);
  std::cout << "========================" << std::endl;
  Cpu Cpu2core(2);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu2core, taskInstances, pool, tdgg, ExeSegs),
      false);
}

TEST(TriggeredDirectedGraphGroup, Test0AddTask) {
  DirectedGraph dg({"A & B -> E", "C -> F", "D -> G", "E -> H", "E & F -> I",
                    "F & G -> J", "H -> K", "I -> L", "I & J-> M", "J -> N",
                    "M -> O", "O -> P", "O -> Q"});
  EXPECT_EQ(dg.getVertex("E")->supplementary_information_.preVertices.size(),
            2);
  TaskBasicInformationPool pool;
  EXPECT_EQ(pool.newTask("A", 2, 6, 0, 0), true);
  EXPECT_EQ(pool.newTask("B", 1, 3, 0, 1), true);
  EXPECT_EQ(pool.newTask("C", 2, 4, 0, 2), true);
  EXPECT_EQ(pool.newTask("D", 1, 5, 0, 3), true);
  EXPECT_EQ(pool.newTask("E", 2), true);
  EXPECT_EQ(pool.newTask("F", 1), true);
  EXPECT_EQ(pool.newTask("G", 1), true);
  EXPECT_EQ(pool.newTask("H", 2), true);
  EXPECT_EQ(pool.newTask("I", 1), true);
  EXPECT_EQ(pool.newTask("J", 1), true);
  EXPECT_EQ(pool.newTask("K", 2), true);
  EXPECT_EQ(pool.newTask("L", 1), true);
  EXPECT_EQ(pool.newTask("M", 1), true);
  EXPECT_EQ(pool.newTask("N", 2), true);
  EXPECT_EQ(pool.newTask("O", 1), true);
  EXPECT_EQ(pool.newTask("P", 2), true);
  EXPECT_EQ(pool.newTask("Q", 1), true);
  EXPECT_EQ(preCheck(dg, pool), true);

  TtsDeadlineParser ttsDeadline({"H : 8", "K : 12", "L : 10", "P : 11",
                                 "Q : 11", "B : 4", "F : 5", "N : 8"});
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);
  EXPECT_EQ(maxFrom(dg.getVertex("E"), pool), dg.getVertex("A"));

  EXPECT_EQ(tdgg.deadlinePropagation(ttsDeadline, pool).first, true);
  pool.dump();
  TaskInstancePool taskInstances;
  TaskInstanceQueue Queue;
  ExecutionSegmentPool ExeSegs;
  LeastWaitingTimeBudgetFirst lwtbf;
  Cpu Cpu5core(5);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu5core, taskInstances, pool, tdgg, ExeSegs),
      true);
  std::cout << "========================" << std::endl;
  Cpu Cpu4core(4);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu4core, taskInstances, pool, tdgg, ExeSegs),
      false);
}

TEST(TriggeredDirectedGraphGroup, Test0LongerPeriod) {
  DirectedGraph dg({"A -> E", "E -> H", "E -> I", "H -> K", "I -> L", "I -> M ",
                    "M -> O", "O -> P", "O -> Q"});
  TaskBasicInformationPool pool;
  EXPECT_EQ(pool.newTask("A", 2, 8, 0, 0), true);
  EXPECT_EQ(pool.newTask("E", 2), true);
  EXPECT_EQ(pool.newTask("H", 2), true);
  EXPECT_EQ(pool.newTask("I", 1), true);
  EXPECT_EQ(pool.newTask("K", 2), true);
  EXPECT_EQ(pool.newTask("L", 1), true);
  EXPECT_EQ(pool.newTask("M", 1), true);
  EXPECT_EQ(pool.newTask("O", 1), true);
  EXPECT_EQ(pool.newTask("P", 2), true);
  EXPECT_EQ(pool.newTask("Q", 1), true);
  EXPECT_EQ(preCheck(dg, pool), true);
  TtsDeadlineParser ttsDeadline(
      {"H : 8", "K : 12", "L : 10", "P : 11", "Q : 11"});
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);
  EXPECT_EQ(tdgg.deadlinePropagation(ttsDeadline, pool).first, true);
  pool.dump();
  TaskInstancePool taskInstances;
  TaskInstanceQueue Queue;
  ExecutionSegmentPool ExeSegs;
  LeastWaitingTimeBudgetFirst lwtbf;
  Cpu Cpu2core(2);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu2core, taskInstances, pool, tdgg, ExeSegs),
      false);
  std::cout << "========================" << std::endl;
  Cpu Cpu3core(3);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu3core, taskInstances, pool, tdgg, ExeSegs),
      true);
}

TEST(TriggeredDirectedGraphGroup, Test0ShorterPeriod) {
  DirectedGraph dg({"A -> E", "E -> H", "E -> I", "H -> K", "I -> L", "I -> M ",
                    "M -> O", "O -> P", "O -> Q"});
  TaskBasicInformationPool pool;
  EXPECT_EQ(pool.newTask("A", 2, 4, 0, 0), true);
  EXPECT_EQ(pool.newTask("E", 2), true);
  EXPECT_EQ(pool.newTask("H", 2), true);
  EXPECT_EQ(pool.newTask("I", 1), true);
  EXPECT_EQ(pool.newTask("K", 2), true);
  EXPECT_EQ(pool.newTask("L", 1), true);
  EXPECT_EQ(pool.newTask("M", 1), true);
  EXPECT_EQ(pool.newTask("O", 1), true);
  EXPECT_EQ(pool.newTask("P", 2), true);
  EXPECT_EQ(pool.newTask("Q", 1), true);
  EXPECT_EQ(preCheck(dg, pool), true);
  TtsDeadlineParser ttsDeadline(
      {"H : 8", "K : 12", "L : 10", "P : 11", "Q : 11"});
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);
  EXPECT_EQ(tdgg.deadlinePropagation(ttsDeadline, pool).first, true);
  pool.dump();
  TaskInstancePool taskInstances;
  TaskInstanceQueue Queue;
  ExecutionSegmentPool ExeSegs;
  LeastWaitingTimeBudgetFirst lwtbf;
  Cpu Cpu4core(4);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu4core, taskInstances, pool, tdgg, ExeSegs),
      true);
  std::cout << "========================" << std::endl;
  Cpu Cpu3core(3);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu3core, taskInstances, pool, tdgg, ExeSegs),
      false);
}

TEST(TriggeredDirectedGraphGroup, Test0AddTaskLongerPeriod) {
  DirectedGraph dg({"A & B -> E", "C -> F", "D -> G", "E -> H", "E & F -> I",
                    "F & G -> J", "H -> K", "I -> L", "I & J-> M", "J -> N",
                    "M -> O", "O -> P", "O -> Q"});
  EXPECT_EQ(dg.getVertex("E")->supplementary_information_.preVertices.size(),
            2);
  TaskBasicInformationPool pool;
  EXPECT_EQ(pool.newTask("A", 2, 12, 0, 0), true);
  EXPECT_EQ(pool.newTask("B", 1, 6, 0, 1), true);
  EXPECT_EQ(pool.newTask("C", 2, 8, 0, 2), true);
  EXPECT_EQ(pool.newTask("D", 1, 10, 0, 3), true);
  EXPECT_EQ(pool.newTask("E", 2), true);
  EXPECT_EQ(pool.newTask("F", 1), true);
  EXPECT_EQ(pool.newTask("G", 1), true);
  EXPECT_EQ(pool.newTask("H", 2), true);
  EXPECT_EQ(pool.newTask("I", 1), true);
  EXPECT_EQ(pool.newTask("J", 1), true);
  EXPECT_EQ(pool.newTask("K", 2), true);
  EXPECT_EQ(pool.newTask("L", 1), true);
  EXPECT_EQ(pool.newTask("M", 1), true);
  EXPECT_EQ(pool.newTask("N", 2), true);
  EXPECT_EQ(pool.newTask("O", 1), true);
  EXPECT_EQ(pool.newTask("P", 2), true);
  EXPECT_EQ(pool.newTask("Q", 1), true);
  EXPECT_EQ(preCheck(dg, pool), true);

  TtsDeadlineParser ttsDeadline({"H : 8", "K : 12", "L : 10", "P : 11",
                                 "Q : 11", "B : 4", "F : 5", "N : 8"});
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);
  EXPECT_EQ(maxFrom(dg.getVertex("E"), pool), dg.getVertex("A"));

  EXPECT_EQ(tdgg.deadlinePropagation(ttsDeadline, pool).first, true);
  pool.dump();
  TaskInstancePool taskInstances;
  TaskInstanceQueue Queue;
  ExecutionSegmentPool ExeSegs;
  LeastWaitingTimeBudgetFirst lwtbf;

  Cpu Cpu3core(3);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu3core, taskInstances, pool, tdgg, ExeSegs),
      true);
  std::cout << "========================" << std::endl;
  Cpu Cpu2core(2);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu2core, taskInstances, pool, tdgg, ExeSegs),
      false);
}

TEST(TriggeredDirectedGraphGroup, Test1) {
  DirectedGraph dg({"B", "C -> F", "D -> G", "F & G -> J", "J -> N"});
  dg.dump();
  EXPECT_EQ(dg.getSourceVerticesName().size(), 3);
  TaskBasicInformationPool pool;
  EXPECT_EQ(pool.newTask("B", 1, 3), true);
  EXPECT_EQ(pool.newTask("C", 2, 4), true);
  EXPECT_EQ(pool.newTask("D", 1, 5), true);
  EXPECT_EQ(pool.newTask("F", 1), true);
  EXPECT_EQ(pool.newTask("G", 1), true);
  EXPECT_EQ(pool.newTask("J", 1), true);
  EXPECT_EQ(pool.newTask("N", 2), true);
  EXPECT_EQ(preCheck(dg, pool), true);

  TtsDeadlineParser ttsDeadline({"B : 4", "F : 5", "N : 8"});
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);
  EXPECT_EQ(tdgg.getSourceVerticesName().size(), 3);
  EXPECT_EQ(maxFrom(dg.getVertex("J"), pool), dg.getVertex("G"));

  EXPECT_EQ(tdgg.deadlinePropagation(ttsDeadline, pool).first, true);
  pool.dump();
  tdgg.dump();
  TaskInstancePool taskInstances;
  TaskInstanceQueue Queue;
  ExecutionSegmentPool ExeSegs;
  LeastWaitingTimeBudgetFirst lwtbf;
  Cpu Cpu3core(3);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu3core, taskInstances, pool, tdgg, ExeSegs),
      true);
  std::cout << "========================" << std::endl;
  Cpu Cpu2core(2);
  EXPECT_EQ(
      lwtbf.schedulable(Queue, Cpu2core, taskInstances, pool, tdgg, ExeSegs),
      false);
}

TEST(DirectedGraph, Test1) {
  DirectedGraph dg({"B ", "B -> ", "A->B", "C", "A -> B"});
  dg.dump();

  EXPECT_EQ(dg.size(), 3);
  EXPECT_EQ(dg.getSourceVerticesName().size(), 2);
  EXPECT_EQ(dg.getIndegree("A"), 0);
  EXPECT_EQ(dg.getIndegree("C"), 0);
  EXPECT_EQ(dg.getIndegree("B"), 1);

  EXPECT_EQ(dg.getOutdegree("A"), 1);
  EXPECT_EQ(dg.getOutdegree("B"), 0);
  EXPECT_EQ(dg.getOutdegree("C"), 0);

  DirectedGraph dg1({"B "});
  EXPECT_EQ(dg1.size(), 1);
  EXPECT_EQ(dg1.getSourceVerticesName().size(), 1);
  EXPECT_EQ(dg1.getIndegree("B"), 0);
  EXPECT_EQ(dg1.getOutdegree("B"), 0);
}