#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2020-02-18 19:56
@edit time: 2020-04-20 21:13
@desc: 将addon的源码安装到原ns3的代码中
"""

import argparse
import json
import os
from pathlib import Path

# 配置执行参数
parser = argparse.ArgumentParser(description='安装参数')
parser.add_argument('--wafdir', default='/ns-3-dev', type=str, help='安装目录')

# build及参数
parser.add_argument('--noconfirm', default=False, action='store_true', help='是否跳过环境检测')
parser.add_argument('--nocopy', default=False, action='store_true', help='是否跳过文件复制')
parser.add_argument('--nowscript', default=False, action='store_true', help='是否跳过wscript覆盖')
parser.add_argument('--norebuild', default=False, action='store_true', help='是否跳过waf rebuild')
parser.add_argument('--noreconf', default=False, action='store_true', help='是否跳过waf reconf')

args = parser.parse_args()

cur_path = Path().resolve()
ns3_path = (cur_path / args.wafdir).resolve()
dir_dict = {
    'action-executor': 'internet',
    'metric-extractor': 'flow-monitor'
}

def error(msg):
    return f"\033[1;31;49m{msg}\033[0m"

def warning(msg):
    return f"\033[0;33;49m{msg}\033[0m"
    
def modend(msg):
    return f"\033[0;32;49m{msg}\033[0m"

def modstart(msg):
    return f"\033[1;34;49m{msg}\033[0m"

def info(msg):
    return f"\033[1;37;49m{msg}\033[0m"

def env_confirm():
    """检查安装环境"""
    # 确保安装目录存在
    print(modstart(f'Checking if dir {ns3_path.resolve()} exists.\n'))
    if not ns3_path.exists():
        raise TypeError(f"{ns3_path.resolve()} not exists!")


def file_copy():
    """进行文件复制操作"""
    print(modstart(f"Copy files to ns3 path.\n"))
    # 将rapidjson复制到ns3文件夹下
    print(f"cp -r rapidjson/ {ns3_path.resolve()}")
    os.system(f"cp -r rapidjson/ {ns3_path.resolve()}")

    # 将ns3src/下文件夹复制到 ns3path/src 对应文件夹下
    src_path = ns3_path / 'src'
    ns3src_path = cur_path / 'ns3-src'

    for module_path in ns3src_path.iterdir():   # 遍历ns3src下的目录
        # 每个module都要被复制到 ns3_path/src 下的对应目录
        print(info(f"copy module {dir_dict[module_path.parts[-1]]}"))
        # 创建文件夹
        mapped_path = ns3src_path / dir_dict[module_path.parts[-1]]
        os.system(f'cp -r {module_path.resolve()} {mapped_path.resolve()}')
        # 拷贝文件夹
        os.system(f"cp -r {mapped_path.resolve()} {src_path.resolve()}")
        # 删除临时文件夹
        os.system(f'rm -r {mapped_path.resolve()}')

    # 将scratch文件夹下cpp部分复制到对应目录下
    scratch_path = ns3_path / 'scratch'
    ns3scratch_path = cur_path / 'ns3-scratch'

    for program_path in ns3scratch_path.iterdir():     # 遍历所有要被复制的模拟器
        # 每个program都要被复制到 ns3_path/scratch 下
        print(f'cp -r {program_path.resolve()} {scratch_path.resolve()}')
        os.system(f'cp -r {program_path.resolve()} {scratch_path.resolve()}')


def wscript_append(waf_script: str, module_path: str):
    """向wscript文件中添加相应文件的编译需求
    @param waf_script: 读取的wafscript文件内容
    @param module_path: 与要修改的wscript对应的module的路径（在addon文件夹下）
    """
    replace_str = ''
    ws_lines = waf_script.split('\n')

    # 获取所有additioal文件
    additional_path = module_path / 'additional.json'
    with additional_path.open('r') as f:
        add_files = json.load(f)

    # 对于不同的新增列表，其对应key是要被添加的目标list
    for target_list in add_files:
        # 对于这个list，遍历新增头文件，并创建相应的append语句
        for add_file in add_files[target_list]:
            has_this_file = False
            # 检查这一行在wscript中是否已经存在
            for ws_line in ws_lines:
                if add_file in ws_line:
                    has_this_file = True
            # 只有没有这行的时候才需要新增语句
            if not has_this_file:
                replace_str += f'''    {target_list}.append("{add_file}")\n'''

    # 如果有要添加的项，则加上头尾。否则用原文替换（即不修改）
    if replace_str:
        replace_str = '    # append compiling for RL methods\n' + replace_str + '\n'

    replace_str += '    bld.ns3_python_bindings()\n'
    new_script = waf_script.replace('    bld.ns3_python_bindings()\n', replace_str)

    return new_script


def wscript_rewrite():
    """修改wscript文件
    将新增的文件写入wscript的编译列表，保证新增文件被编译
    """

    print(modstart(f"Rewrite all wscripts.\n"))
    # 遍历ns3-src下的module，修改对应的wafscript
    src_path = ns3_path / 'src'
    ns3src_path = cur_path / 'ns3-src'

    for addon_module_path in ns3src_path.iterdir():   # 遍历ns3src下的目录
        module_name = dir_dict[addon_module_path.parts[-1]]         # 获取module的相对路径，即module名称
        wscript_path = src_path / module_name / 'wscript'     # 获取wscript在ns3_path下的对应路径
        print(info(f"rewrite wcsript of module {module_name}"))
        # 先读取这个文件的内容
        waf_script = (wscript_path.read_text())
        # 添加编译信息，获取新的scrpte字符串
        new_wscript = wscript_append(waf_script, addon_module_path)
        # 新字符串写入文件
        wscript_path.write_text(new_wscript)


def waf_reconf():
    """执行waf configure指令"""
    # 定位waf
    waf_path = ns3_path / 'waf'
    os.chdir(ns3_path.resolve())
    # 执行指令
    print(info(f'\n{waf_path} -d debug --enable-tests configure\n'))
    os.system(f'{waf_path} -d debug --enable-tests configure')


def waf_rebuild():
    """执行waf build指令"""
    # 定位waf
    waf_path = ns3_path / 'waf'
    os.chdir(ns3_path.resolve())
    # 执行指令
    print(info(f'\n\n{waf_path} build\n\n'))
    os.system(f'{waf_path} build')


def install():
    """
    安装 ns3-addon，包括：
    - 确认目标文件夹存在
    - ns3源文件拷贝
    - 更新相关module的wscript文件
    - 重新执行waf 的 configure 和 build 指令
    - 安装ns3gym包（因为依赖configure产生的文件所以放在最后）
    """
    if not args.noconfirm:
        env_confirm()
    if not args.nocopy:
        file_copy()
    if not args.nowscript:
        wscript_rewrite()
    if not args.noreconf:
        waf_reconf()
    if not args.norebuild:
        waf_rebuild()
    print("\033[1;36;49mAddon files install finished.\033[0m\n")


if __name__ == '__main__':
    install()
