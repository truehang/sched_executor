#include "preprocess.hpp"

#include <cstdint>
#include <map>
#include <string>
extern "C" {
#include "internal_pthread.h"
#include "internal_sched.h"
#include "posix_shm.h"
}

namespace gstone {
namespace exec {
std::mutex globalScheduleMutex;

std::string StringOperation::rmWhiteSpace(const std::string &le) {
  static std::set<char> ignored{' '};
  std::string tmp(le);
  for (auto it = tmp.begin(); it != tmp.end();) {
    if (ignored.find(*it) != ignored.end()) {
      it = tmp.erase(it);
    } else
      ++it;
  }
  return tmp;
}

std::pair<std::string, std::string> StringOperation::splitWithDemiliter(
    const std::string &long_expression, const std::string &demiliter) {
  auto reduced = std::move(rmWhiteSpace(long_expression));
  int ret = reduced.rfind(demiliter);
  if (ret == -1) return std::make_pair(reduced, "");
  return std::make_pair(reduced.substr(0, ret),
                        reduced.substr(ret + demiliter.size()));
}

DirectedGraph::DirectedGraph(const std::vector<std::string> &relations) {
  for (auto &str : relations) {
    this->addRelation(str);
  }
  updateVertexPointer();
}

DirectedGraph::DirectedGraph(
    const struct preschedule_dependency_array *dependencies) noexcept {
  for (int i = 0; i < dependencies->size; ++i) {
    this->addRelation(dependencies->array[i].data);
  }
  updateVertexPointer();
}

void DirectedGraph::addRelation(const std::string &relation) {
  std::set<std::string> dependency_set;
  std::string name;
  tie(dependency_set, name) = parseRelation(relation);
  /* Add Vertex */
  addVertex(name);
  for (auto it = dependency_set.begin(); it != dependency_set.end(); ++it) {
    addVertex(*it);
    /* add PostVertice */
    addPostVertex(*it, name);
  }
  /* Set pre vertices (a vertex set) */
  setPreVertices(name, dependency_set);
}

void DirectedGraph::updateVertexPointer() noexcept {
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto postVs = it->second->getPostVertices();
    for (auto &pv : postVs) {
      it->second->supplementary_information_.postVertices.push_back(
          this->at(pv).get());
    }
    auto preVs = it->second->getPreVertices();

    for (auto &pv : preVs) {
      it->second->supplementary_information_.preVertices.push_back(
          this->at(pv).get());
    }
  }
  vertex_pointer_update_ = true;
}

/* Format: Ta &[|] Tb ... -> Ttarget
e.g. (A & B) | C -> D, or  B | C -> E */
std::pair<std::set<std::string>, std::string> DirectedGraph::parseRelation(
    const std::string &relation) {
  std::pair<std::set<std::string>, std::string> ret;
  static std::set<char> delimiter{'(', ')', '&', '|'};
  std::set<std::string> dependency_set;

  std::string expr;
  std::string name;
  std::tie(expr, name) = StringOperation::splitWithDemiliter(relation, "->");

  auto tmp_begin = expr.begin();
  bool start_match = false;
  for (auto it = expr.begin(); it != expr.end(); ++it) {
    if (!start_match && delimiter.find(*it) == delimiter.end()) {
      start_match = true;
      tmp_begin = it;
    } else if (start_match && delimiter.find(*it) != delimiter.end()) {
      start_match = false;
      dependency_set.emplace(tmp_begin, it);
    }
  }

  if (start_match) {
    dependency_set.emplace(tmp_begin, expr.end());
  }
  return std::make_pair(std::move(dependency_set), std::move(name));
}

void DirectedGraph::addVertex(const std::string &name) {
  if (name == "") return;
  this->emplace(name, std::make_shared<DirectedVertex>(name));
}

void DirectedGraph::addPostVertex(const std::string &name,
                                  const std::string &post_vertex) {
  if (name == "" || post_vertex == "") return;
  this->operator[](name)->addPostVertex(post_vertex);
}

void DirectedGraph::setPreVertices(const std::string &name,
                                   const std::set<std::string> &dependency) {
  if (name == "" || dependency.empty()) return;
  this->operator[](name)->SetPreVertices(dependency);
}

std::set<std::string> DirectedGraph::getSourceVerticesName() const {
  std::set<std::string> res;
  for (auto it = this->begin(); it != this->end(); ++it) {
    if (it->second->getIndegree() == 0) res.emplace(it->first);
  }
  return res;
}

void DirectedGraph::dependencyReadyReset() {
  for (auto it = this->begin(); it != this->end(); ++it) {
    it->second->dependencyReadyReset();
  }
}

std::set<DirectedVertex *> DirectedGraph::getSourceVertices() const noexcept {
  std::set<DirectedVertex *> ret;
  for (auto it = this->begin(); it != this->end(); ++it) {
    if (it->second->getIndegree() == 0) ret.emplace(it->second.get());
  }
  return ret;
}

DirectedVertex *DirectedGraph::getVertex(const std::string &name) const {
  if (this->find(name) == this->end()) {
    return nullptr;
  }
  return this->at(name).get();
}

void DirectedGraph::dump() {
  for (auto it = this->begin(); it != this->end(); ++it) {
    if (it->second->getIndegree() > 0) {
      std::cout << "(";
      for (auto &a : it->second->getPreVertices()) std::cout << a << " ";
      std::cout << ")->";
    }
    std::cout << it->first;
    if (it->second->getOutdegree() > 0) {
      std::cout << "->(";
      for (auto &a : it->second->getPostVertices()) std::cout << a << " ";
      std::cout << ")";
    }
    if (std::next(it, 1) == this->end())
      std::cout << "." << std::endl;
    else
      std::cout << ", ";
  }
}

void TaskBasicInformation::dumpFieldName() {
  std::cout << "name, burstTime, period, cumulativeBurstTime, "
               "latestStartExecutionTimeOffset"
            << std::endl;
}

void TaskBasicInformation::dumpValue() const noexcept {
  std::cout << this->name << ", " << this->burstTime << ", " << this->period
            << ", " << this->cumulativeBurstTime << ", "
            << this->latestStartExecutionTimeOffset << std::endl;
}

TaskInstance::TaskInstance(const int instanceId, TaskBasicInformation *taskInfo,
                           int nowTime, int arrivalTimeOffset) {
  this->name = taskInfo->name;
  this->instanceId = instanceId;
  this->status = TaskInstanceStatus::WAITING;
  this->arrivalTime = nowTime;
  this->arrivalTimeOffset = arrivalTimeOffset;
  this->completionTime = 0;
  this->burstTime = taskInfo->burstTime;
  this->cumulativeExecutionTime = 0;
  this->turnaroundTime = 0;
  this->responseTime = 0;
  this->waitingTime = 0;
  this->waitingTimeBudget =
      std::min(taskInfo->deadline - taskInfo->burstTime,
               taskInfo->latestStartExecutionTimeOffset - arrivalTimeOffset);
  this->deadline = this->waitingTimeBudget + this->burstTime;
}

void TaskInstance::dump(bool from_cpu) const noexcept {
  std::cout << "name: " << this->name << ", arrivalTime: " << this->arrivalTime
            << ", arrivalTimeOffset: " << this->arrivalTimeOffset
            << ", waitingTime: " << this->waitingTime
            << ", waitingTimeBudget: " << this->waitingTimeBudget
            << ", burstTime: " << this->burstTime;
  if (!from_cpu) {
    std::cout << ", cumulativeExecutionTime: " << this->cumulativeExecutionTime;
  }
  std::cout << std::endl;
}

bool TaskBasicInformationPool::newTask(const std::string &name, int burstTime,
                                       int period, int firstArrivalTime,
                                       int taskId) {
  bool result;
  auto ptr = std::make_shared<TaskBasicInformation>(name, burstTime, period,
                                                    firstArrivalTime, taskId);
  std::tie(std::ignore, result) = this->emplace(name, ptr);
  return result;
}

bool TaskBasicInformationPool::hasTask(const std::string &name) const noexcept {
  return this->find(name) != this->end();
}

int TaskBasicInformationPool::updateLetoWithTtsDeadline(
    const std::string &name, int tts_deadline) noexcept {
  /* No such name found.*/
  if (!hasTask(name)) {
    return -3;
  }

  auto task = this->at(name).get();
  /* Invalid deadline*/
  if (tts_deadline < task->cumulativeBurstTime) return -2;

  int SuggestedLeto = tts_deadline - task->burstTime;
  if (SuggestedLeto < task->latestStartExecutionTimeOffset) {
    task->latestStartExecutionTimeOffset = SuggestedLeto;
    /* Update leto*/
    return task->latestStartExecutionTimeOffset;
  }

  /*Deadline propagation ends*/
  return -1;
}

void TaskBasicInformationPool::dump() const noexcept {
  if (this->empty()) {
    std::cout << "Task Pool is empty." << std::endl;
  } else {
    TaskBasicInformation::dumpFieldName();
    for (auto it = this->begin(); it != this->end(); ++it) {
      it->second->dumpValue();
    }
  }
}

TaskBasicInformationPool::TaskBasicInformationPool(
    const struct preschedule_task_info_array *tasks) noexcept {
  for (int i = 0; i < tasks->size; ++i) {
    this->newTask(tasks->array[i].name, tasks->array[i].bt, tasks->array[i].p);
  }
}

TtsDeadlineParser::TtsDeadlineParser(
    const std::vector<std::string> &tts_deadlines) noexcept {
  this->clear();
  for (auto &str : tts_deadlines) {
    addDeadline(str);
  }
}
bool TtsDeadlineParser::addDeadline(const std::string &str) noexcept {
  std::string key;
  std::string value;
  std::tie(key, value) = StringOperation::splitWithDemiliter(str, ":");
  if (this->find(key) != this->end()) {
    return false;
  }
  if (value == "") {
    return false;
  }
  int tts_deadline;
  try {
    tts_deadline = std::stoi(value);
  } catch (const std::exception &e) {
    return false;
  }
  if (tts_deadline <= 0) {
    return false;
  }

  this->operator[](key) = tts_deadline;
  return true;
}

TtsDeadlineParser::TtsDeadlineParser(
    const struct preschedule_tts_deadline_array *tts_deadlines) noexcept {
  for (int i = 0; i < tts_deadlines->size; ++i) {
    addDeadline(tts_deadlines->array[i].data);
  }
}

void ProcessQueue::addSet(const std::set<DirectedVertex *> &set) {
  for (auto &vertex : set) {
    this->emplace(vertex);
  }
}

int ProcessQueue::DoneThenAddSubsequent(DirectedVertex *vertex) {
  int count = 0;
  auto &postVs = vertex->supplementary_information_.postVertices;
  for (auto &v : postVs) {
    v->dependencyReadyAddOne();
    if (v->isReady()) {
      ++count;
      this->emplace(v);
    }
  }
  return count;
}

DirectedVertex *maxFrom(DirectedVertex *vertex,
                        TaskBasicInformationPool &taskPool) {
  auto preVs = vertex->supplementary_information_.preVertices;
  if (preVs.empty()) return nullptr;

  auto chooseMax = [&taskPool](DirectedVertex *a, DirectedVertex *b) {
    auto name_a = a->getName();
    auto name_b = b->getName();
    if ((taskPool.at(name_a)->period > taskPool.at(name_b)->period) ||
        ((taskPool.at(name_a)->period == taskPool.at(name_b)->period) &&
         (taskPool.at(name_a)->cumulativeBurstTime >=
          taskPool.at(name_b)->cumulativeBurstTime))) {
      return true;
    }
    return false;
  };

  std::sort(preVs.begin(), preVs.end(), chooseMax);
  return preVs.front();
}

/* Every vertex on the directed graph should have task parameters.*/
bool preCheck(const DirectedGraph &dg,
              const TaskBasicInformationPool &taskPool) {
  for (auto it = dg.begin(); it != dg.end(); ++it) {
    if (!taskPool.hasTask(it->first)) {
      return false;
    }
  }
  return true;
}

void TriggeredDirectedGraphGroup::generate(DirectedGraph &dg,
                                           TaskBasicInformationPool &taskPool) {
  this->clear();
  dg.dependencyReadyReset();

  if (!preCheck(dg, taskPool)) {
    return;
  }
  ProcessQueue que;
  que.addSet(dg.getSourceVertices());
  while (!que.empty()) {
    auto vertex = que.front();
    auto vertexName = vertex->getName();
    que.pop();

    /* Dependent Task*/
    if (vertex->getIndegree() > 0) {
      auto thePreVertex = maxFrom(vertex, taskPool);
      auto thePreVertexName = thePreVertex->getName();
      taskPool[vertexName]->cumulativeBurstTime +=
          taskPool[thePreVertexName]->cumulativeBurstTime;
      taskPool[vertexName]->period = taskPool[thePreVertexName]->period;
      taskPool[vertexName]->deadline = taskPool[vertexName]->period;
      /* Add a relation ship thePreVertexName -> vertexName*/
      addRelation(thePreVertexName + "->" + vertexName);
    } else {
      addVertex(vertexName);
    }

    que.DoneThenAddSubsequent(vertex);
  }
  updateVertexPointer();
}

std::pair<bool, std::vector<std::string>>
TriggeredDirectedGraphGroup::deadlinePropagation(
    const TtsDeadlineParser &tts_deadline, TaskBasicInformationPool &taskPool) {
  std::vector<std::string> invalidDeadline;
  for (auto it = tts_deadline.begin(); it != tts_deadline.end(); ++it) {
    /* Update leto with tts deadline*/
    int leto = taskPool.updateLetoWithTtsDeadline(it->first, it->second);
    auto vertex = getVertex(it->first);

    if (leto == -3 || vertex == nullptr) {
      continue;
    }

    if (leto == -2) {
      invalidDeadline.push_back(it->first);
      continue;
    }

    /* Deadline propagation ends if leto equals -1*/
    while (leto != -1) {
      /*Source task exit while loop*/
      if (vertex->supplementary_information_.preVertices.empty()) {
        break;
      }
      vertex = vertex->supplementary_information_.preVertices.front();
      leto = taskPool.updateLetoWithTtsDeadline(vertex->getName(), leto);
    }
  }

  if (invalidDeadline.empty()) {
    return std::make_pair(true, invalidDeadline);
  }
  return std::make_pair(false, invalidDeadline);
}

int TaskInstanceQueue::sumWTB() const noexcept {
  if (this->empty()) return INT32_MAX;
  int val = 0;
  for (auto it = this->begin(); it != this->end(); ++it) {
    val += (*it)->waitingTimeBudget;
  }
  return val;
}

void TaskInstanceQueue::merge(const TaskInstanceQueue &anotherQueue) {
  for (auto &taskInstance : anotherQueue) {
    this->push_back(taskInstance);
  }
}

void TaskInstanceQueue::addNewTasks(const TaskInstanceQueue &another, int now) {
  std::lock_guard<std::mutex> lk(globalScheduleMutex);
  updateWaitingTimeBudeget(now);
  if (another.empty()) return;
  merge(another);
  sort();
}

bool TaskInstanceQueue::updateWaitingTimeBudeget(int now) {
  if (now <= globalLastTime) {
    return false;
  }
  int deltaTime = now - globalLastTime;
  globalLastTime = now;
  /* Update wt in Queue*/
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto &task = (*it);
    task->waitingTime += deltaTime;
    task->waitingTimeBudget -= deltaTime;
    if (task->waitingTimeBudget < 0) {
      globalSchedulable = false;
      task->status = TaskInstance::TaskInstanceStatus::FAILED;
    }
  }
  return true;
}

void TaskInstanceQueue::sort() noexcept {
  std::sort(this->begin(), this->end(),
            [](const TaskInstance *a, const TaskInstance *b) {
// #define DEDICATESORT /* when wtb is the same, less burstTime higher prior*/
#ifdef DEDICATESORT
              return a->waitingTimeBudget < b->waitingTimeBudget ||
                     ((a->waitingTimeBudget == b->waitingTimeBudget) &&
                      (a->burstTime < b->burstTime));
#else
              return a->waitingTimeBudget < b->waitingTimeBudget;
#endif
            });
}

void TaskInstanceQueue::dumpTimeOut() const noexcept {
  if (this->empty()) {
    std::cout << "Queue is empty." << std::endl;
  }
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto &task = (*it);
    if (task->waitingTimeBudget < 0) {
      task->dump();
    }
  }
}

void TaskInstanceQueue::dump(bool from_cpu) const noexcept {
  if (this->empty()) {
    std::cout << "Queue is empty." << std::endl;
  }
  for (auto it = this->begin(); it != this->end(); ++it) {
    (*it)->dump(from_cpu);
  }
}

TaskInstanceQueue::TaskInstanceQueue(
    const std::unordered_map<TaskInstance *, CpuBindingInformation> &deque) {
  for (auto it = deque.begin(); it != deque.end(); ++it) {
    this->push_back(it->first);
  }
}

ExecutionSegment *ExecutionSegmentPool::newSeg(int startTime, int endTime,
                                               int instanceId,
                                               int coreIndex) noexcept {
  static int ExeSegId = 0;
  ++ExeSegId;
  auto newExeSeg = std::make_shared<ExecutionSegment>(
      ExeSegId, startTime, endTime, instanceId, coreIndex);
  this->emplace(ExeSegId, newExeSeg);
  return newExeSeg.get();
}

void CpuCore::reset() {
  for (auto it = this->arranged_.begin(); it != this->arranged_.end(); ++it)
    *it = -1;
  this->in_use_ = false;
  this->startTime_ = 0;
  this->expectedYieldTime_ = 0;
  this->currentTaskInstance_ = nullptr;
}

int CpuCore::yield(int now, ExecutionSegmentPool &ExeSegs) noexcept {
  if (now < startTime_) {
    return 1;
  }
  if (currentTaskInstance_ == nullptr || !in_use_) {
    return 0;
  }
  if (now > startTime_) {
    int deltaTime = now - startTime_;
    currentTaskInstance_->cumulativeExecutionTime += deltaTime;
    if (currentTaskInstance_->cumulativeExecutionTime <
        currentTaskInstance_->burstTime) {
      currentTaskInstance_->status = TaskInstance::TaskInstanceStatus::WAITING;
    } else {
      currentTaskInstance_->status = TaskInstance::TaskInstanceStatus::DONE;
      currentTaskInstance_->completionTime = now;
      currentTaskInstance_->turnaroundTime =
          currentTaskInstance_->waitingTime + currentTaskInstance_->burstTime;
    }

    currentTaskInstance_->executionSegmentArray.push_back(ExeSegs.newSeg(
        startTime_, now, currentTaskInstance_->instanceId, index_));
  }
  startTime_ = 0;
  expectedYieldTime_ = 0;
  currentTaskInstance_ = nullptr;
  in_use_ = false;
  return 0;
}

int CpuCore::take(TaskInstance *taskInstance, int now) noexcept {
  if (taskInstance == nullptr || in_use_) {
    return 1;
  }

  startTime_ = now;
  expectedYieldTime_ =
      now + taskInstance->burstTime - taskInstance->cumulativeExecutionTime;
  currentTaskInstance_ = taskInstance;
  taskInstance->status = TaskInstance::TaskInstanceStatus::EXECUTING;
  taskInstance->responseTime = now;
  in_use_ = true;
  return 0;
}

Cpu::Cpu(int cores, int sim_time) : core_num_(cores) {
  for (int i = 0; i < core_num_; ++i) this->emplace_back(i, sim_time);
}

void Cpu::reset() {
  for (auto it = this->begin(); it != this->end(); ++it) it->reset();
  inExecution_.clear();
  globalLastTime = 0;
}

int Cpu::nextYieldTime() const noexcept {
  int time = INT32_MAX;
  for (auto it = inExecution_.begin(); it != inExecution_.end(); ++it) {
    if (it->second.expectedYieldTime < time) {
      time = it->second.expectedYieldTime;
    }
  }
  return time;
}

std::vector<TaskInstance *> Cpu::yield(int now, ExecutionSegmentPool &ExeSegs) {
  std::vector<int> coreId;
  std::vector<TaskInstance *> tasks;
  for (auto it = inExecution_.begin(); it != inExecution_.end();) {
    if (it->second.expectedYieldTime == now) {
      coreId.push_back(it->second.cpuCoreId);
      tasks.push_back(it->first);
      it = inExecution_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto &a : coreId) {
    (this->begin() + a)->yield(now, ExeSegs);
  }
  return tasks;
}
bool Cpu::updateWtb(int now) noexcept {
  for (auto it = inExecution_.begin(); it != inExecution_.end(); ++it) {
  }

  if (now <= globalLastTime) {
    return false;
  }
  int deltaTime = now - globalLastTime;
  globalLastTime = now;
  /* Update wt in Queue*/
  for (auto it = inExecution_.begin(); it != inExecution_.end(); ++it) {
    auto task = it->first;
    task->waitingTimeBudget -= deltaTime;
  }
  return true;
}

TaskInstanceQueue Cpu::giveUp(int now, ExecutionSegmentPool &ExeSegs) {
  TaskInstanceQueue unfinished;
  for (auto it = this->begin(); it != this->end(); ++it) {
    if (it->expectedYieldTime_ > now) {
      unfinished.push_back(it->currentTaskInstance_);
    }
    it->yield(now, ExeSegs);
  }
  inExecution_.clear();
  return unfinished;
}

std::vector<int> Cpu::available() const noexcept {
  std::vector<int> core_idx;
  for (auto it = this->begin(); it != this->end(); ++it) {
    if (!it->inUse()) {
      core_idx.push_back(it - this->begin());
    }
  }
  return core_idx;
}

bool Cpu::swapOutIn(TaskInstanceQueue &out, TaskInstanceQueue &in,
                    ExecutionSegmentPool &ExeSegs, int timeNow) noexcept {
  int retval = 0;
  for (auto &task : out) {
    /* Stop current task, add a segement*/
    int cpuCoreId = inExecution_.at(task).cpuCoreId;
    retval += findCore(cpuCoreId)->yield(timeNow, ExeSegs);
    inExecution_.erase(task);
  }
  auto idleCoreId = available();
  /* Arrange task whose preferred core is idle*/
  for (auto taskIt = in.begin(); taskIt != in.end();) {
    auto segs = (*taskIt)->executionSegmentArray;
    bool found = false;
    if (!segs.empty()) {
      int preferredCoreId = segs.back()->cpuCoreId;
      auto itId = std::find(std::begin(idleCoreId), std::end(idleCoreId),
                            preferredCoreId);
      if (itId != std::end(idleCoreId)) {
        retval += findCore(preferredCoreId)->take((*taskIt), timeNow);
        int expYT =
            (*taskIt)->burstTime - (*taskIt)->cumulativeExecutionTime + timeNow;
        inExecution_.emplace(std::piecewise_construct, std::make_tuple(*taskIt),
                             std::make_tuple(expYT, preferredCoreId));
        idleCoreId.erase(itId);
        found = true;
      }
    }
    if (found) {
      taskIt = in.erase(taskIt);
    } else {
      ++taskIt;
    }
  }
  /* Arrange the rest*/
  auto coreIdIt = idleCoreId.begin();
  for (auto taskIt = in.begin(); taskIt != in.end(); ++taskIt) {
    retval += findCore(*coreIdIt)->take((*taskIt), timeNow);
    int expYT =
        (*taskIt)->burstTime - (*taskIt)->cumulativeExecutionTime + timeNow;
    inExecution_.emplace(std::piecewise_construct, std::make_tuple(*taskIt),
                         std::make_tuple(expYT, (*coreIdIt)));
    ++coreIdIt;
  }
  // // out.clear();
  // in.clear();
  return (retval == 0);
}

Cpu::iterator Cpu::findCore(int core_index) {
  return (this->begin() + core_index);
}

static int Gcd(int a, int b) {
  // base condition
  if (b == 0) {
    return a;
  }
  return Gcd(b, a % b);
}

static int LcmOfArray(const std::vector<int> &vec) {
  // LCM(n1, n2)= (n1*n2)/GCD(n1, n2)
  // LCM(n1, n2, n3)= (n1*n2*n3)/GCD(n1, n2, n3)
  if (vec.empty()) {
    return 0;
  }
  int lcm = vec[0];
  int gcd = vec[0];
  // loop through the array to find GCD
  // use GCD to find the LCM
  for (auto i = 1U; i < vec.size(); ++i) {
    gcd = Gcd(vec[i], lcm);
    lcm = (lcm * vec[i]) / gcd;
  }
  return lcm;
}

void Simulator::init(const TaskBasicInformationPool &taskPool,
                     TriggeredDirectedGraphGroup &tddg) {
  auto sourceTaskNames = tddg.getSourceVerticesName();
  /* Get LCM of source period*/
  std::vector<int> sourceTaskPeriod;
  for (auto &name : sourceTaskNames) {
    auto sourceTask = taskPool.at(name).get();
    sourceTaskPeriod.emplace_back(sourceTask->period);
  }
  int lcm = LcmOfArray(sourceTaskPeriod);
  LCM_ = lcm;
  glabalSourceTaskTime.clear();
  /* Add source task arrival time*/
  for (auto &name : sourceTaskNames) {
    auto sourceTask = taskPool.at(name).get();
    int offset = sourceTask->firstArrivalTime;
    int period = sourceTask->period;
    while (offset < lcm) {
      glabalSourceTaskTime.emplace_back(offset, name);
      offset += period;
    }
  }
  std::sort(
      glabalSourceTaskTime.begin(), glabalSourceTaskTime.end(),
      [](const std::pair<int, std::string> &a,
         const std::pair<int, std::string> &b) { return a.first < b.first; });
}

std::pair<int, TaskInstanceQueue> Simulator::nextSourceTaskInstance(
    TaskInstancePool &taskInstancePool, TaskBasicInformationPool &taskPool) {
  TaskInstanceQueue que;

  if (glabalSourceTaskTime.empty()) {
    return std::make_pair(INT32_MAX, que);
  }

  int time = glabalSourceTaskTime.front().first;

  while (!glabalSourceTaskTime.empty() &&
         glabalSourceTaskTime.front().first == time) {
    auto tmp = glabalSourceTaskTime.front();
    auto taskInfo = taskPool.at(tmp.second).get();
    auto newInstance = taskInstancePool.newTaskInstance(taskInfo, time, 0);

    que.emplace_back(newInstance);
    taskInfo->instanceArray.emplace_back(newInstance);
    glabalSourceTaskTime.pop_front();
  }
  return std::make_pair(time, que);
}

TaskInstanceQueue Simulator::nextTaskInstance(
    const std::vector<TaskInstance *> &taskInstanceDone, int time,
    TaskInstancePool &taskInstancePool, TaskBasicInformationPool &taskPool,
    TriggeredDirectedGraphGroup &tddg) {
  TaskInstanceQueue que;
  if (taskInstanceDone.empty()) {
    return que;
  }
  for (auto &instance : taskInstanceDone) {
    auto name = instance->name;
    auto postVertices =
        tddg.at(name).get()->supplementary_information_.postVertices;
    if (postVertices.empty()) continue;
    for (auto &postV : postVertices) {
      auto taskInfo = taskPool.at(postV->getName()).get();
      auto newInstance = taskInstancePool.newTaskInstance(
          taskInfo, time,
          instance->arrivalTimeOffset + instance->turnaroundTime);
      que.emplace_back(newInstance);
      taskInfo->instanceArray.emplace_back(newInstance);
    }
  }

  return que;
}

bool LeastWaitingTimeBudgetFirst::schedulable(TaskInstanceQueue &Queue,
                                              Cpu &Cpu,
                                              TaskInstancePool &TaskInstances,
                                              TaskBasicInformationPool &Tasks,
                                              TriggeredDirectedGraphGroup &Tdgg,
                                              ExecutionSegmentPool &ExeSegs) {
  Queue.clear();
  TaskInstances.clear();
  ExeSegs.clear();
  TaskInstanceQueue lastLcmTail;
  Simulator sim;
  int sumWTB;
  int loop_cnt = 0;
  int sumWTBLast;
  do {
    Queue.paramsReset();
    Cpu.reset();
    lastLcmTail = Queue;
    sumWTBLast = lastLcmTail.sumWTB();
    sim.init(Tasks, Tdgg);
    int lcm = sim.LCM_;
    int nextSourceTime;
    TaskInstanceQueue sourceQueue;
    std::tie(nextSourceTime, sourceQueue) =
        sim.nextSourceTaskInstance(TaskInstances, Tasks);

    for (int i = nextSourceTime; i <= lcm;) {
      TaskInstanceQueue ret;
      if (i == Cpu.nextYieldTime()) {
        ret = sim.nextTaskInstance(Cpu.yield(i, ExeSegs), i, TaskInstances,
                                   Tasks, Tdgg);
      }
      if (nextSourceTime == i) {
        ret.merge(sourceQueue);
        std::tie(nextSourceTime, sourceQueue) =
            sim.nextSourceTaskInstance(TaskInstances, Tasks);
      }
      Queue.addNewTasks(ret, i);
      Cpu.updateWtb(i);
      bool preemptive = true;
      if (schedule(Queue, Cpu, ExeSegs, i, preemptive) == false) {
        std::cout << "During " << ++loop_cnt
                  << " iteration: Unscheduable--------->" << std::endl;
        std::cout << "now: " << i
                  << ", time out task in ready Queue: " << std::endl;
        Queue.dumpTimeOut();
        std::cout << "<--------------------" << std::endl;
        return false;
      }

      i = std::min(nextSourceTime, Cpu.nextYieldTime());
    }

    TaskInstanceQueue A1 = Cpu.giveUp(lcm, ExeSegs);
    Queue.updateWaitingTimeBudeget(lcm);
    Queue.merge(A1);
    Queue.sort();
    sumWTB = Queue.sumWTB();
    std::cout << "After " << ++loop_cnt << " iteration:--------->" << std::endl;
    std::cout << "now: " << lcm << ", LCM Tail sumWTB: " << sumWTB
              << " task count: " << Queue.size()
              << ", last sumWTB: " << sumWTBLast
              << " task count: " << lastLcmTail.size() << std::endl;
    Queue.dump();
    std::cout << "<--------------------" << std::endl;
  } while (Queue.size() > lastLcmTail.size() || sumWTB < sumWTBLast);
  (void)loop_cnt;
  return true;
}

bool LeastWaitingTimeBudgetFirst::schedule(TaskInstanceQueue &Queue, Cpu &Cpu,
                                           ExecutionSegmentPool &ExeSegs,
                                           int now, bool preemptive) noexcept {
  /* Do the LWTBF scheduling*/
  if (Queue.empty()) return true;
  auto idleCpuCores = Cpu.available();
  if (!Queue.globalSchedulable) {
    return false;
  }

  /* Check CPU*/
  // auto Queue = GlobalTaskInstanceWaitingQueue::getInstance();
  TaskInstanceQueue A1(Cpu.inExecution_);
  A1.sort();
#if 1
  std::cout << "now: " << now << ", idle core count: " << idleCpuCores.size()
            << ", task in CPU:" << std::endl;
  A1.dump(true);
  std::cout << "task in ready Queue:" << std::endl;
  Queue.dump();
#endif
  TaskInstanceQueue A_in;
  TaskInstanceQueue A_out;
  while (!Queue.empty()) {
    auto i = Queue.front();
    if (!idleCpuCores.empty()) {
      /* Take idle core*/
      idleCpuCores.pop_back();
      A_in.push_back(i);
      Queue.pop_front();
    } else if (preemptive) {
      /* Preemption Satisfied*/
      if (!A1.empty() && i->waitingTimeBudget < A1.back()->waitingTimeBudget) {
        A_in.push_back(i);
        Queue.pop_front();
        A_out.push_back(A1.back());
        A1.pop_back();
      } else {
        break;
      }
    } else {
      break;
    }
  }

  if (!A_in.empty()) {
    Cpu.swapOutIn(A_out, A_in, ExeSegs, now);
  }
  if (!A_out.empty()) {
    Queue.merge(A_out);
    Queue.sort();
  }

  return true;
}

extern "C" int preprocess(union data *data) {
  DirectedGraph dg(&(data->pre_data.dependencies));
  dg.dump();
  TaskBasicInformationPool pool(&(data->pre_data.task_infos));

  if (!preCheck(dg, pool)) return -1;

  TtsDeadlineParser ttsDeadline(&(data->pre_data.tts_deadlines));
  TriggeredDirectedGraphGroup tdgg;
  tdgg.generate(dg, pool);

  if (!tdgg.deadlinePropagation(ttsDeadline, pool).first) return -1;
#define TEST
#ifdef TEST
  pool.dump();
  tdgg.dump();
#endif
  /*fill task array */
  struct task_array tasks;
  struct source_task source_tasks;
  struct task_dependency dependencies;
  struct preschedule_task_info_array *pre_info_tasks =
      &(data->pre_data.task_infos);
  struct priority_queue running_tasks; /*This is a max heap*/
  memset(&running_tasks, 0, sizeof(running_tasks));
  running_tasks.max_heap = 1;

  std::map<std::string, uint8_t> task_and_idx;
  for (uint8_t i = 0; i < pre_info_tasks->size; ++i) {
    task_and_idx.emplace(pre_info_tasks->array[i].name, i);
  }

  if (!task_and_idx.size()) return -1;
  tasks.size = pre_info_tasks->size;
  for (uint8_t i = 0; i < pre_info_tasks->size; ++i) {
    /* set task param*/
    memcpy(&(tasks.array[i].thread), &(pre_info_tasks->array[i].thread),
           sizeof(struct thread_node));

    memcpy(tasks.array[i].name, pre_info_tasks->array[i].name, TASKNAMELENGTH);
    TaskBasicInformation *cur_task =
        pool.at(pre_info_tasks->array[i].name).get();
    tasks.array[i].bt = pre_info_tasks->array[i].bt;
    int tmp_period = cur_task->period;
    tasks.array[i].d = pre_info_tasks->array[i].d > 0
                           ? pre_info_tasks->array[i].d
                           : tmp_period;
    tasks.array[i].leto = cur_task->latestStartExecutionTimeOffset;
    tasks.array[i].last_cpu_index = 0;
    /* reset run time params of task*/
    struct task_node *cur_node = &(tasks.array[i]);
    memset(&(cur_node->active), 0,
           sizeof(*cur_node) -
               ((uint8_t *)&(cur_node->active) - (uint8_t *)cur_node));

    /* Arrange dependency*/
    auto &post_v =
        tdgg.getVertex(pre_info_tasks->array[i].name)->getPostVertices();
    for (auto &str : post_v) {
      dependencies.connection[i][task_and_idx.at(str)] = 1;
    }

    /* find source task*/
#ifdef TEST
    printf("name %s, tid: %d, indgree %ld, sig %d\n",
           pre_info_tasks->array[i].name, pre_info_tasks->array[i].thread.tid,
           tdgg.getIndegree(pre_info_tasks->array[i].name),
           pre_info_tasks->array[i].trigger_sig);
#endif

    /* check whether curretn task is source task*/
    if (tdgg.getIndegree(pre_info_tasks->array[i].name) == 0) {
      source_tasks.array[source_tasks.size].idx_in_task_arr = i;
      source_tasks.array[source_tasks.size].trigger_sig =
          pre_info_tasks->array[i].trigger_sig;
      tasks.array[i].is_source = 1;
      ++(source_tasks.size);
      set_task_prior(tasks.array[i].thread.tid, 3); /* source task triger*/
    } else {
      tasks.array[i].is_source = 0;
      set_task_prior(tasks.array[i].thread.tid,
                     0); /* none-source task not ready*/
    }
  }
  /* set task array, source tasks and dependencies, and reset the rest*/
  memset(data, 0, sizeof(union data));
  memcpy(&(data->ready_data.tasks), &tasks, sizeof(struct task_array));
  memcpy(&(data->ready_data.source_tasks), &source_tasks,
         sizeof(struct source_task));
  memcpy(&(data->ready_data.dependencies), &dependencies,
         sizeof(struct task_dependency));
  memcpy(&(data->ready_data.running_tasks), &running_tasks,
         sizeof(struct priority_queue));
  return 0;
}

}  // namespace exec
}  // namespace gstone