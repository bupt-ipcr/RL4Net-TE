# wjwgym(暂名): 强化学习工具包

基于`Pytorch`与`OpenAI Gym`实现强化学习训练涉及到：  

- 强化学习智能体编写
- 神经网络创建
- gym环境编写  

wjwgym提供了常用强化学习智能体 `DQN` `DDPG` `QLearning` 的pytorch实现，完成了其中的通用部分。可以作为智能体编写的参考，也可以直接作为基类用于创建自己的强化学习智能体。同时实现了简单的神经网络和gym环境，可以作为实现的参考。  

## Agents  

`wjwgym.agents` 中实现了`DQNBase` `DDPGBase` 和 `LinearBase` 三个基类，分别作为`DQN` `DDPG` 和 `QLearning`/`Sarsa` 的通用抽象。

- DQN_base  
  或称`DoubleDQN`，在标准DQN基础上增加了经验回放和fixed_target。实现了标准的`训练` `获取动作` `添加记录到经验回放池`等方法，同时提供了自己实现神经网络构建的接口。  
- DDPG_base  
  实现了DDPG通用的`训练` `获取动作` `添加记录到经验回放池`方法，同时提供Actor和Critic神经网络创建的接口。有默认的噪声函数实现，也支持自己实现噪声函数。  
- Linear_base
  实现了线性学习通用的QTable，但训练函数需要自己创建。  

## Models  

`wjwgym.models` 中实现了几种简单的神经网络，用于创建示例。  

## Envs  

`wjwgym.envs` 中实现了简单的环境用于创建示例，在`wjwgym.envs._init__`中给出了将环境注册到gym中的方法。需要注意：  

- 需要将包安装到site-packages下  
- 需要 `import wjwgym` 或者 `from wjwgym import envs`，`__init__`文件中的注册代码才会生效。
