
# 编译

放到 `vpp` 源码 `src/plugins/` 目录下

```
make build

make run
```

# 配置

```
plugins {

    plugin flowfive_plugin.so { enable }

    # plugin flowfive_plugin.so { disable }

}

flowfive {
    rsyslog-host 127.0.0.1         # 设置 rsyslog 服务器主机 IP
    rsyslog-port 514               # 设置 rsyslog 服务器端口
    rsyslog-facility 19            # 设置 rsyslog local3 = 19
    rsyslog-severity 6             # 设置 rsyslog info = 6
    cache-size 8192                # 设置 缓存缓存数量
}
```

# 启动

#### 开启 `GigabitEthernet0/13/0` 接口 `flowfive`

```
# vppctl

DBGvpp# set flowfive interface GigabitEthernet0/13/0 enable
Setting FlowFive for interface GigabitEthernet0/13/0 to enable
Updated interface GigabitEthernet0/13/0 to enabled
Current interfaces:
  Interface GigabitEthernet0/13/0: enabled
DBGvpp#
```

#### 查看启用 `flowfive` 接口

```
# vppctl

DBGvpp# show flowfive interface
Enabled FlowFive on the following interfaces:
  Interface: GigabitEthernet0/13/0
DBGvpp# 
```

#### 查看 `flowfive` rsyslog 配置

```
# vppctl

DBGvpp# show flowfive rsyslog
FlowFive Rsyslog Configuration:
  host: 10.238.40.250
  port: 5200
  facility: 19            # local3
  severity: 6             # info
  log-type: 1             # 1 文本
  cache-size: 8192        # 日志队列长度
  cache-size-cursize: 0   # 当前内存日志数量
DBGvpp#
```

#### 修改 `flowfive` rsyslog 配置

```
# 修改 rsyslog host 地址
DBGvpp# set rsyslog host 192.168.1.253

# 查看 rsyslog 配置 host 已生效
DBGvpp# show flowfive rsyslog
FlowFive Rsyslog Configuration:
  host: 192.168.1.253
  port: 5200
  facility: 19
  severity: 6
  log-type: 1
  cache-size: 8192
  cache-size-cursize: 0
```


#### 输出日志

| 源IP          | 目的IP  | 源端口 | 目的端口 | 协议 | 长度 | 时间戳 | 五分钟时间戳 |
| ------------- | ------- | ------ | -------- | ---- | ---- | ---------- | ---------- |
| 192.168.1.130 | 1.1.1.1 | 0      | 0        | 1    | 84   | 1717389644 | 1717389600 |
| 192.168.1.130 | 1.1.1.1 | 48632  | 12345    | 6    | 60   | 1717389643 | 1717389600 |


```
192.168.1.130 1.1.1.1 0 0 1 84 1717389644 1717389600
192.168.1.130 1.1.1.1 48632 12345 6 60 1717389643 1717389600
```


# rsyslog

```
template( name="t_output" type="string" string="%msg% %fromhost-ip%\n" )
template( name="b_file" type="string" string="/tmp/vpp-%$now%.log" )
```
