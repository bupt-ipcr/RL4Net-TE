#!/usr/bin/env python
# coding=utf-8
"""
@create time: 2019-11-21 11:17
@author: Jiawei Wu
@edit time: 2020-01-15 17:01
@file: /test.py
"""

import time
import gym
import wjwgym
from wjwgym.agents import LinearBase


class QLearning(LinearBase):
    """基于线性QTable创建的QLearning类"""
    def __init__(self, actions, learning_rate=0.01, reward_decay=0.9, e_greedy=0.9):
        super(QLearning, self).__init__(actions, e_greedy, learning_rate, reward_decay)

    def learn(self, s, a, r, d, s_):
        """
        Q-Learning计算Q估计的时候使用Qmax
        """
        self.check_state_exist(s_)
        q_predict = self.q_table.loc[s, a]
        if not d:
            q_target = r + self.gamma * self.q_table.loc[s_, :].max()  # next state is not terminal
        else:
            q_target = r  # next state is terminal
        self.q_table.loc[s, a] += self.lr * (q_target - q_predict)  # update


def rl_loop(env, agent):
    """
    @description: 
    @param env: 传入的环境对象
    @param agent: 传入的智能体对象 
    @return: 
    """
    for episode in range(10):
        # initial observation
        state = env.reset()

        while True:
            # fresh env
            # env.render()
            time.sleep(0.1)
            # RL choose action based on observation
            action = agent.choose_action(str(state))

            next_state, reward, done, step_count = env.step(action)

            # QLearning
            agent.learn(str(state), action, reward, done, str(next_state))

            # swap observation
            state = next_state

            # break while loop when end of this episode
            if done:
                # env.render()
                time.sleep(0.2)
                print('steps: ', step_count, 'reward: ', reward)
                break
    agent.print_table()

    # end of game
    print('game over')


if __name__ == "__main__":
    env = gym.make('Maze-v0')
    RL = QLearning(actions=list(range(env.action_space.n)))
    rl_loop(env, RL)
