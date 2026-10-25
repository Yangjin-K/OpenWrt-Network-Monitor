# OpenWrt Network Monitor & Firewall Manager
## 基于 OpenWrt 的网络流量监控与防火墙管理系统

本项目基于嵌入式 Linux 系统（OpenWrt），开发了一套轻量级的 Web 网络管理工具。包含基于 C 语言的底层流量数据采集、基于 Web 的实时流量展示面板，以及通过 Shell 脚本封装的防火墙规则动态配置功能。

## 技术栈
- **系统环境**：OpenWrt 24.10.0 (x86_64) / VMware 虚拟机 / WSL 交叉编译
- **底层开发**：C 语言 / Linux Shell / Linux 内核 `/proc/net/dev` 接口
- **前端**：HTML / JavaScript (定时轮询 AJAX 请求)
- **网络安全**：`nftables` 防火墙规则配置

## 文件结构说明
| 文件名              | 说明                                    |
| ------------------- | --------------------------------------- |
| `traffic_monitor.c` | 流量监控核心 C 程序源码（读取内核数据） |
| `index.html`        | Web 前端实时展示页面                    |
| `traffic.sh`        | 封装防火墙配置命令的 CGI 脚本           |

## 核心功能与实现

### 1. 流量监控（C 语言底层实现）
通过读取 Linux 内核维护的 `/proc/net/dev` 文件，实时获取 `eth0` 网卡的收发字节数，每 2 秒刷新一次。
```bash
# 编译方式
gcc -o traffic_monitor traffic_monitor.c

# 运行
./traffic_monitor
```

### 2. Web 界面展示

利用 OpenWrt 自带的 `uhttpd` 服务，前端页面通过 JavaScript 定时请求后端数据，每 3 秒自动刷新流量面板。

bash

```
# 启动 HTTP 服务
/etc/init.d/uhttpd start
# 访问 http://192.168.5.10
```



### 3. 防火墙规则管理

通过 Shell 脚本封装 `nftables` 命令，实现规则的安全增删改查。

bash

```
# 查看当前规则
nft list ruleset

# 添加规则（例如禁止访问 80 端口）
nft add rule inet fw4 input tcp dport 80 drop

# 清空所有规则
nft flush ruleset
```



## 开发难点与避坑经验

1. **嵌入式环境交叉编译**：OpenWrt 默认的 Overlay 空间仅有约 70MB，无法直接安装 `gcc` 编译 C 程序。本项目通过在 Windows 下使用 WSL (Ubuntu 22.04) 搭建交叉编译工具链，成功编译出可在 OpenWrt 运行的二进制文件。
2. **网络连通性调试**：在 VMware 虚拟网络配置中，遇到虚拟机无法上网的问题。经排查为网络配置文件中缺少 gateway 和 dns 参数，且实际 NAT 网关地址与默认配置不符，经反复调试后解决。

## 实验环境

- 虚拟机软件：VMware Workstation 17 Pro
- 操作系统：OpenWrt 24.10.0 (x86_64)
- 交叉编译环境：WSL Ubuntu 22.04 + gcc 11.4.0

