#!/bin/zsh

# 设置你的 Git 仓库路径
REPO_PATH="/root/my_xtp/src"

# 切换到仓库目录
cd "$REPO_PATH" || exit 1

# 添加所有更改
git add .


# 可选日志
echo "Git add & commit executed at $(date)" >> /root/my_xtp/src/log/git_auto_add.log

