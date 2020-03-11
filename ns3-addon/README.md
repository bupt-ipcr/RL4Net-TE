<!--
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-02-22 20:58
 * @file: /README.md
 -->
DRL-TE
============  

将强化学习应用到流量工程。

依赖包
============

网络仿真部分使用  [ns-3](https://www.nsnam.org/) 作为网络仿真模拟器

为了简化ns3和RL部分的交互，使用了 ns3-gym 作为接口，可以访问 [Github仓库](https://github.com/LampV/ns3gym-brief) 安装

为了方便编写RL代码，使用了工具库，可以访问 [Github仓库](https://github.com/LampV/Reinforcement-Learning) 安装

安装
============

1. 下载代码

   ```bash
   git clone http://gitlab.local/incubator/drl_te/ -b drl-te
   cd drl_te
   ```

2. 将代码安装到ns3目录

   ```bash
   python setup.py --build 
   ```

   默认安装目录是 `/ns-3-dev`