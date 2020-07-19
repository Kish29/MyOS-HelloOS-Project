#! /usr/bin/python3 

'''
该脚本会剔除未做出修改的文件，通过最后一次修改时间来判断
所以只要最后一次修改时间不同该文件就会不一样
'''

import GetValidFile
import pymysql
from base64 import b64decode
import pdb
import getpass

# 现获取当前目录下所有有效文件包括代码本身以及最新修改时间
GetValidFile.recursive_get_valid_file('.')

def print_information():
    for key, value in GetValidFile.valid_files.items():
        print("文件名:\n", key, '\n', '内容:\n', value, '\n')

    for key, value in GetValidFile.file_last_adapt_time.items():
        print("最近修改时间:\n", key, '\n', '文件名:\n', value, '\n')

# 麻烦一下，用户自己输入，这样更安全
# user_info = []
# 
# user_info_file = open('/root/.mysql_user_info', 'r')
# 
# for line in user_info_file:
#     user_info.append(line)
# 
# user_info_file.close()
# 
# user, password  = b64decode(user_info[0]).decode(), b64decode(user_info[1]).decode()

user = input("user>")
password = getpass.getpass("password>")

# establish connection
try:
    connect = pymysql.connect(
                host="127.0.0.1",
                port=3306,
                user=user,
                password=password,
                db="my_projects_codes"
            )
except Exception as e:
     print("Connect to Mysql failed!", e)
     exit(-1)


# 可执行sql语句光标对象
cursor = connect.cursor()

# 先跟据文件的最后修改时间剔除不需要添加的数据
# 注意这个sql语句的类型转换
sql = "select file_name, cast(last_adapt_date as char) from hello_os_codes;"
cursor.execute(sql)
res = cursor.fetchall()

# 注意这个遍历的迭代器写法
for (fn, tm) in res:
    # 如果该文件存在于数据库中
    if fn in GetValidFile.file_last_adapt_time:
        # 并且记录的修改时间于本地即将提交的文件的最后修改记录时间相同
        if tm == GetValidFile.file_last_adapt_time[fn]:
            del GetValidFile.file_last_adapt_time[fn]
            del GetValidFile.valid_files[fn]

if len(GetValidFile.valid_files) == 0:
    print("All files up to date, exit!")
    exit(0)

# 显示一下所有要提交到数据库的文件信息
print("This is all files you'll store in the database:\n")
print_information()
confirm = input("Are you sure to store them? [y]es, [n]o>")

if confirm == 'n' or confirm == 'N':
    exit(-1)
elif confirm == 'y' or confirm == 'Y':
    pass 
else:
    print("Invalid input!")
    exit(-1)

# 插入数据
for key, value in GetValidFile.valid_files.items():
    sql = "insert into hello_os_codes values(\"%s\",\"%s\",\"%s\")"
    # 记住特殊字符要进行处理，单引号替换成↑箭头，双引号替换成↓箭头
    value = value.replace("'", "↑").replace('"', '↓')
    sql = sql % (key, value, GetValidFile.file_last_adapt_time[key])
    cursor.execute(sql)
    connect.commit()

cursor.close()
connect.close()
