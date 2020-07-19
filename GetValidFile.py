#! /usr/lib/python3 

import os 
import re
from datetime import datetime
 
# 有效文件的文件名和数据
valid_files = {}
code = ''
file_last_adapt_time = {}

# 递归获取有效文件
def recursive_get_valid_file(dir_path):
    current_dir = os.listdir(dir_path)
    # 注意顺序，先里用目录名列出列出所有文件名后再进入该目录中
    os.chdir(dir_path)
    for i in range(len(current_dir)):
        # 如果是目录，继续向下递归寻找有效文件
        if os.path.isdir(current_dir[i]):
            recursive_get_valid_file(current_dir[i])
        else:
            if is_valid_file(current_dir[i]):
                # 打开文件，获取文件数据以及最后修改时间
                file = open(current_dir[i], 'r')    
                code = ''
                for line in file:
                    code += line
                merge_time = datetime.fromtimestamp(os.path.getmtime(current_dir[i])).strftime("%Y-%m-%d %H:%M:%S")
                file_last_adapt_time[current_dir[i]] = merge_time
                valid_files[current_dir[i]] = code
                file.close    
    # 当前目录遍历完成后，返回上一级
    os.chdir("..")



pattern = ['.+\.cpp$', '.+\.py$', '.+\.java$', '.+\.c$', '.+\.s$', '.+\.asm$', '.+\.sh$', '.+\.cs$', '^makefile$', 'readme.md']
def is_valid_file(dir_name):
    value = False 
    for i in range(len(pattern)):
        # 忽略大小写
        value = re.match(pattern[i], dir_name, re.I)
        if value:
            return value 

    return value


# if __name__ == "__main__":
#     recursive_get_valid_file('.')
#
#     for key, value in valid_files.items():
#         print("文件名:\n", key, '\n', '内容:\n', value, '\n')
# 
#     del valid_files['loader.asm']
# 
#     for key, value in valid_files.items():
#         print("文件名:\n", key, '\n', '内容:\n', value, '\n')
# 
#     for key, value in file_last_adapt_time.items():
#         value = datetime.fromtimestamp(value).strftime("%Y-%m-%d %H:%M:%S")
#         print("文件名:\n", key, '\n', '最近修改时间:\n', value, '\n')
