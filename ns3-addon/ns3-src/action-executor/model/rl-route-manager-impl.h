/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-25 17:35
 * @desc: RL概率路由计算的实现
 */


#ifndef RL_ROUTE_MANAGER_IMPL_H
#define RL_ROUTE_MANAGER_IMPL_H

#include <stdint.h>
#include <list>
#include <queue>
#include <map>
#include <vector>
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "rl-router-interface.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

namespace ns3 {

// PAP means shortest path first
// thus probability all path can be called PAP
const double PAP_INFINITY = -1.0; //!< "infinite" distance between nodes

class RLCandidateQueue;
class Ipv4RLRouting;

/**
 * @brief 使用RL方法进行路由计算的数据库
 *
 * 数据库中以节点序号作为节点标识，用pair<nodeIndex, nodeIndex>作为一对节点的标识  
 * 
 * 数据库记录记下内容：  
 * - 邻接矩阵：节点之间的邻接关系  
 * - 可达矩阵： 节点之间的可达关系    
 * - 权重矩阵： 节点往下一跳发送包的权重  
 * 
 * 数据库对外提供以下接口：
 * - 使用邻接矩阵初始化数据库内容
 * - 使用给定的矩阵更新权重矩阵  
 * - 判断两节点之间是否邻接
 * - 判断两节点之间是否可达
 * - 判断两节点通过指定节点转发是否可行
 * - 获得到指定下一跳节点通过的出口接口编号
 * 
 * 其中：  
 * 1. 可达矩阵是由邻接矩阵通过wallshell算法得到的  
 * 
 */

class RLRoutingDB
{
public:
  typedef uint32_t NodeId;
  typedef std::pair<NodeId, NodeId> Edge; //!< 用一对router的标识表示一条边
  typedef std::pair<Ipv4Address, uint32_t> NextNode; //<! 由下一跳IP与出口if构成的下一节点信息
  typedef std::map<Edge, NextNode> NextNodeMatrix; //!< src和next确定的下一节点信息表
  typedef std::map<Edge, int> Matrix; //!< 用边-标记组成的map 表示 src 与 dst 之间关系
  typedef std::map<Edge, double> WeightMatrix; //!< 用边-标记组成的map 表示 src 与 dst 之间关系

  /**
   * @brief 空RL Routing Database 的构造方法
   */
  RLRoutingDB ();

  /**
 * @brief 删除空的 RL Routing Database.
 *
 * The database map is walked and all of the Link State Advertisements stored
 * in the database are freed; then the database map itself is clear ()ed to
 * release any remaining resources.
 */
  ~RLRoutingDB ();

  /**
   * @brief 初始化RLRoutingDatabase
   *
   *  给定list形式的邻接矩阵和node数目，计算并设置Matrix形式的邻接矩阵。
   *  通过邻接矩阵使用wallshell算法计算可达矩阵，并设置。
   *  计算OutputInterfaceMap，并设置。
   *
   *  @param adjacencyArray 邻接关系的数组
   *  @param nodes 被使用的节点容器
   */
  void Initialize (int *adjacencyArray, NodeContainer nodes);

  /**
   * @brief 更新权重矩阵
   * 
   * 遍历传入数组形式的权重矩阵，读取其中的权值并设置到Matrix形式的
   * m_weightMatrix中。
   * 
   * @param weightArray 
   * @param nodeNum
   */
  void SetWeightMatrix (double *weightArray, uint32_t nodeNum);

  /**
   * @brief 获取一条边的权重
   * 
   * 从src往nextHop转发的权重，也可以理解为与src关联的这条边的权重
   * 注意如果src与nextHop不是邻接关系，则权重应该衡为0
   * 
   * @param edge 一条有向边，由src节点和dst节点确定
   * @return double 这条边的权重
   */
  double GetWeight (Edge edge);

  /**
   * @brief 获取一条边的权重
   * 参见GetWeight(Edge edge)
   * 
   * @param src 
   * @param nextHop 
   * @return double 
   */
  double GetWeight (NodeId src, NodeId nextHop);

  /**
   * @brief 获取出口接口
   * 
   * 从src向nextHop发包的接口
   * 注意当src与nextHop不是邻接关系的时候接口为0
   * 
   * @param edge 边是一对节点(src, nextHop)的组合
   * @return NextNode 源节点src到下一跳节点信息，包括 IP和outIf
   */
  NextNode GetNextNode (Edge edge);

  /**
   * @brief 获取出口接口
   * 参见GetOutputInterface(Edge edge)
   * 
   * @param src 源节点
   * @param dst 目的节点
   * @return NextNode 
   */
  NextNode GetNextNode (NodeId src, NodeId nextHop);

  /**
   * @brief 判两个节点是否相邻
   * 
   * @param edge 边是一对节点(src, dst)的组合
   * @return int -1: 不相邻; 0: 是自己; 1: 相邻
   */
  int IsAdjacency (Edge edge);
  /**
   * @brief 判断两个节点是否相邻
   * 参见IsAdjacency(Edge edge)
   * 
   * @param src scource node
   * @param dst destination node
   * @return int -1: 不相邻; 0: 是自己; 1: 相邻
   */
  int IsAdjacency (NodeId src, NodeId dst);

  /**
   * @brief 判两个节点是否可达
   * 
   * @param edge 边是一对节点(src, dst)的组合
   * @return int -1: 不可达; 0: 是自己; 1: 可达
   */
  int IsReachable (Edge edge);

  /**
   * @brief 判断两个节点是否可达
   * 参见IsReachable(Edge edge)
   * 
   * @param src scource node
   * @param dst destination node
   * @return int -1: 不可达; 0: 是自己; 1: 可达
   */
  int IsReachable (NodeId src, NodeId dst);

  /**
   * @brief 判断src通过next到达dst是否是有效的路径
   * 
   * 如果src和next之间是相邻的(adjacenct)，且next和dst之间是可达的
   * (reachable)，则src->next->dst 是有效的路径
   * 关系表如下：
   * adjacenct  reachable   valid   explain
   *    -1        any         -2    下一跳无法到达
   *    0         any         0     下一跳是自身
   *    1         -1          -1    无法到达终点
   *    1         0           1     可行，且一跳就到达
   *    1         1           2     可行
   * @param src
   * @param dst
   * @param next
   * @return int 
   */
  int IsValidPath (NodeId src, NodeId next, NodeId dst);

private:
  NextNodeMatrix m_nextNodeMatrix; //!< 记录 src-dst对应的源出口if
  Matrix m_adjacencyMatrix; //!< 记录 src-dst之间是否相邻
  Matrix m_reachableMatrix; //!< 记录 src-dst之间是否可达
  WeightMatrix m_weightMatrix; //!< 记录 src向nextHop 转发的权重

  /**
   * @brief 设置邻接矩阵
   * 
   * 遍历传入数组形式的邻接矩阵，读取其中的邻接情况并设置到Matrix形式的
   * m_adjacencyMatrix中。
   * 
   * @param adjacencyArray 数组形式的邻接矩阵
   * @param nodeNum 
   */
  void SetAdjacencyMatrix (int *adjacencyArray, uint32_t nodeNum);

  /**
   * @brief 计算并设置可达矩阵
   * 
   * 通过传入的邻接矩阵，使用wallshell算法计算可达矩阵。然后按照Matrix的
   * 格式设置到私有变量中。
   * 
   * @param adjacencyArray 
   * @param nodeNum 
   */
  void CalcReachableMatrix (int *adjacencyArray, uint32_t nodeNum);

  /**
   * @brief 初始化出口接口记录
   * 
   * 通过遍历检测所有的节点、设备及信道，判断下一跳与出口接口的关系，记录到映射表
   * 
   * @param nodes 节点的容器
   */
  void InitNextNodeMatrix (NodeContainer nodes);

  /**
   * @brief 初始化权重矩阵
   * 
   * 遍历所有可能的src-nextHop对，检查是否邻接
   * 如果邻接，则将权重初始化为1，否则初始化为0
   * 注意src-src的对权重初始化为0，即默认不能向自己转发
   * 
   * @param nodeNum 节点数目
   */
  void InitWeightMatrix (uint32_t nodeNum);

  /**
 * @brief 对于 RLRouteManagerLSDB 的拷贝方法是不被允许的。因此显式声明拷贝方法以避免
 * 出现编译器默认隐式拷贝导致的出错。
 * @param lsdb 要拷贝的目标
 */
  RLRoutingDB (RLRoutingDB &rldb);

  /**
 * @brief 对于 RLRouteManagerLSDB 的拷贝方法是不被允许的。因此显式声明拷贝方法以避免
 * 出现编译器默认隐式拷贝导致的出错。
 * @param lsdb 要拷贝的对象
 * @returns 拷贝结果
 */
  RLRoutingDB &operator= (RLRoutingDB &rldb);
};

/**
 * @brief 基于强化学习、概率路由的路由实现
 *
 * 概率路由不计算最短路径，而是保留所有可能可行的路径
 * 各条路径的转发概率由边的权重决定.其中，权重是对所有边设置，不考虑是否可以到达最终节点
 * 详细的说明参见  Ipv4RLRouting  
 * 
 * Manager集中地计算所有的路由表，然后将每一条路由表项发送给对应src节点的路由层协议
 * 
 * Manager对外提供以下接口
 * - void BuildRLRoutingDatabase (int *adjacencyArray);
 *   由外部传入邻接矩阵，用于内部数据库的初始化
 * 
 * - void SetWeightMatrix (double *weightArray);   
 *   由外部传入权重矩阵，设置权重矩阵
 * 
 * - void CalculateRoutes ();
 *   计算并下发路由表
 * 
 *
 */
class RLRouteManagerImpl
{
public:
  // 记录整条路径信息的路由链表

  RLRouteManagerImpl ();
  virtual ~RLRouteManagerImpl ();

  /**
   * @brief 删除所有路由
   * 
   * 在计算路由表并下发之前，为了避免受到原来的路由表影响，应该先清空
   * 遍历所有节点的所有路由表项，依次删除
   * 其中，删除的时候为了避免修改正在遍历的表，应该遍历NRoutes，删除(0)
   * 
   */
  virtual void DeleteRoutes();

  /**
   * @brief 构建RL路由数据库
   * 
   * 通过邻接矩阵即可构建路由关系，包括邻接、可达、转发可达
   * 调用DB提供的init方法即可实现
   * 
   * @param adjacencyArray 
   * @param nodes
   */
  virtual void BuildRLRoutingDatabase (int *adjacencyArray, NodeContainer nodes);

  /**
   * @brief 设置权重矩阵
   * 
   * 传入一个数组作为参数，用以设置权重。
   * 通过调用Database的SetWeightMatrix() 函数实现
   * 
   * @param weightrray 传入的权重矩阵
   */
  virtual void SetWeightMatrix (double *weightArray);

  /**
   * @brief 计算并下发路由表
   * 
   * 遍历所有节点，为每个节点计算能到达的IP、对应下一跳、对应权重
   * 计算完节点路由表之后将其下发到对应的节点上
   * 
   */
  virtual void CalculateRoutes ();

  /**
   * @brief Debug时获取m_rldb对象
   * 
   */
  RLRoutingDB* DebugGetRLDB();

private:
  NodeContainer m_nodes;
  /**
 * @brief RLRouteManagerImpl copy construction is disallowed.
 * There's no  need for it and a compiler provided shallow copy would be 
 * wrong.
 *
 * @param srmi object to copy from
 */
  RLRouteManagerImpl (RLRouteManagerImpl &srmi);

  /**
 * @brief RL Route Manager Implementation assignment operator is
 * disallowed.  There's no  need for it and a compiler provided shallow copy
 * would be hopelessly wrong.
 *
 * @param srmi object to copy from
 * @returns the copied object
 */
  RLRouteManagerImpl &operator= (RLRouteManagerImpl &srmi);

  RLRoutingDB *m_rldb; //!< the Link State DataBase (LSDB) of the RL Route Manager
};

} // namespace ns3

#endif /* RL_ROUTE_MANAGER_IMPL_H */
