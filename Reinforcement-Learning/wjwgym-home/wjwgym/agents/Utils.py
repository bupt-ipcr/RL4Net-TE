#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-04 10:40
@edit time: 2020-01-14 17:10
@file: /exp_replay.py
"""

import torch
from torch.autograd import Variable
import numpy as np


class ExpReplay:
    """
    使用类似Q-Learning算法的DRL通用的经验回放池
    1. 经验回放池的一条记录
        (state, action, reward, done, next_state) 或简写为 (s, a, r, d, s_)
        其中done的意义在于，当状态为done的时候，Q估计=r，而不是Q估计=(r + gamma * Q'max)
        显然，一条记录的维度是 dim(s) + dim(a) + dim(r) + dim(d) + dim(s_) = 2 * n_states + n_actions + 2
    2. 经验回放池的参数
        - n_states: state的维度
        - n_actions: action的维度
        - exp_size: 经验回放池的容量上限。达到这个上限之后，后续的记录就会覆盖之前的记录。
            默认值是: 1000
        - MIN_MEN: 经验回放池输出的最小数目。记录数目超过阈值之后，获取batch才会获得输出
            默认值是: exp_size//10
        - batch_size: 一个batch的大小。
            默认值是: 32
    3. 经验回放池的功能
        - 创建经验回放池对象
            >>> self.memory = ExpReplay(n_states,  n_actions, exp_size=exp_size, exp_thres=exp_thres)
        - 添加一条记录
            >>> self.memory.add_step(s, a, r, d, s_)
            将这条记录放到经验回放池的顺序位置上，例如：
                第一条记录在位置0，第2条记录在位置1，第exp_size+1条记录在位置0
        - 获取一个batch用于训练
            >>> s, a, r, d, s_ = memery.get_batch_splited()
        - 获取一个已经被转为pytorch Variable的batch用于训练
            >>> CUDA = torch.cuda.is_available()
            >>> batch = self.memory.get_batch_splited_tensor(CUDA, batch_size)
    """

    def __init__(self, n_states, n_actions, exp_size=1000, exp_thres=None, batch_size=32):
        """初始化经验回放池"""
        # 参数复制与定义
        self.n_states, self.n_actions = n_states, n_actions
        if not exp_thres:
            exp_thres = exp_size // 10
        self.exp_size, self.exp_thres = exp_size, exp_thres
        self.batch_size = batch_size
        # 初始化经验回放池
        exp_dim = n_states * 2 + n_actions + 2
        self.expreplay_pool = np.zeros((exp_size, exp_dim))
        # 初始化记录位置
        self.exp_index = 0

    def add_step(self, *step):
        """
        为经验回放池增加一步，我们约定“一步”包括s, a, r, d, s_五个部分
        将这一步的信息整合为一条记录，放置到exp_index指定的位置。之后，exp_index以exp_size为模+1
        即后来的记录会覆盖掉最早的记录

        @param *step: 接收一条需要被添加到经验回放池的记录，要求按照s, a, r, d, s_的顺序给出
        """
        # 构建 step
        s, a, r, d, s_ = step
        step = np.hstack((s.reshape(-1), a, [r], [d], s_.reshape(-1)))
        # step 添加到经验回放池
        index = self.exp_index % self.exp_size
        self.expreplay_pool[index] = step
        # 更改exp_index位置，并确保不会int溢出
        self.exp_index += 1
        if self.exp_index > 2 * self.exp_size:
            self.exp_index -= self.exp_size

    def get_batch(self, batch_size=None):
        """
        从回放池中获取一个batch
        如果经验回放池大小未达到输出阈值则返回None

        @param batch_size: 一个batch的大小，若不指定则按经验回放池的默认值
        @return 一个batch size 的记录
        """
        batch_size = batch_size if batch_size else self.batch_size
        # 超过输出阈值的时候才返回batch
        if self.exp_index >= self.exp_thres:
            expreplay_pool_size = min(self.exp_index, self.exp_size)
            choice_indexs = np.random.choice(expreplay_pool_size, batch_size)
            return self.expreplay_pool[choice_indexs]
        else:
            return None

    def get_batch_splited(self, batch_size=None):
        """
        将batch按照s, a, r, d, s_的顺序分割好并返回

        @param batch_size: 一个batch的大小，若不指定则按经验回放池的默认值
        @return cur_states, actions, rewards, dones, nexe_states: 
            按照 (s, a, r, d, s_) 顺序分割好的一组batch 
        """
        batch = self.get_batch(batch_size)
        if batch is None:
            return batch
        else:
            cur_states = batch[:, :self.n_states]
            actions = batch[:, self.n_states: self.n_states + self.n_actions].astype(int)
            rewards = batch[:, self.n_states + self.n_actions: self.n_states + self.n_actions + 1]
            dones = batch[:, -self.n_states - 1: -self.n_states].astype(int)  # 将是否结束按int类型读取，结束则为1，否则为0
            nexe_states = batch[:, -self.n_states:]
            return cur_states, actions, rewards, dones, nexe_states

    def get_batch_splited_tensor(self, CUDA, batch_size=None, dtype=torch.FloatTensor):
        """
        将batch分割并转换为tensor之后返回

        @param CUDA: 是否使用GPU，这决定了返回变量的设备类型
        @param batch_size: 一个batch的大小，若不指定则按经验回放池的默认值
        @param dtype: 返回变量的数据类型，默认为Float
        @return cur_states, actions, rewards, dones, nexe_states: 
            按照 (s, a, r, d, s_) 顺序分割好且已经转为torch Variable的一组batch 
        """
        batch = self.get_batch_splited(batch_size)
        if batch is None:
            return batch
        else:
            return (torch.from_numpy(ndarray).type(dtype).cuda() for ndarray in batch) if CUDA else (torch.from_numpy(ndarray).type(dtype) for ndarray in batch)


class OUProcess(object):
    """Ornstein-Uhlenbeck process"""

    def __init__(self, x_size, mu=0, theta=0.15, sigma=0.3):
        self.x = np.ones(x_size) * mu
        self.x_size = x_size
        self.mu = mu
        self.theta = theta
        self.sigma = sigma

    def noise(self):
        dx = self.theta * (self.mu - self.x) + self.sigma * np.random.randn(self.x_size)
        self.x = self.x + dx
        return self.x


def soft_update(target, source, tau):
    for target_param, param in zip(target.parameters(), source.parameters()):
        target_param.data.copy_(
            target_param.data * (1.0 - tau) + param.data * tau
        )
