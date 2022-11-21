cgroup.event_control       #用于eventfd的接口
memory.usage_in_bytes      #显示当前已用的内存
memory.limit_in_bytes      #设置/显示当前限制的内存额度
memory.failcnt             #显示内存使用量达到限制值的次数
memory.max_usage_in_bytes  #历史内存最大使用量
memory.soft_limit_in_bytes #设置/显示当前限制的内存软额度
memory.stat                #显示当前cgroup的内存使用情况
memory.use_hierarchy       #设置/显示是否将子cgroup的内存使用情况统计到当前cgroup里面
memory.force_empty         #触发系统立即尽可能的回收当前cgroup中可以回收的内存
memory.pressure_level      #设置内存压力的通知事件，配合cgroup.event_control一起使用
memory.swappiness          #设置和显示当前的swappiness
memory.move_charge_at_immigrate #设置当进程移动到其他cgroup中时，它所占用的内存是否也随着移动过去
memory.oom_control         #设置/显示oom controls相关的配置
memory.numa_stat           #显示numa相关的内存

sudo /home/ylzhao/miniconda3/envs/entest/bin/python3 main.py

https://lessisbetter.site/2020/09/01/cgroup-3-cpu-md/