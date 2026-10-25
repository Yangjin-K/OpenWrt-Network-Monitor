/**
 * 流量监控程序 - OpenWrt版本
 * 编译命令: x86_64-linux-gnu-gcc -static -o traffic_monitor traffic_monitor.c -lpcap
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// 流量统计结构体
typedef struct flow_entry {
    char src_ip[20];
    char dst_ip[20];
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    int rx_packets;
    int tx_packets;
    time_t last_seen;
    struct flow_entry *next;
} flow_entry_t;

flow_entry_t *flow_table = NULL;
int flow_count = 0;

// 查找或创建流记录
flow_entry_t* find_or_create_flow(const char *src, const char *dst) {
    flow_entry_t *curr = flow_table;
    while (curr) {
        if (strcmp(curr->src_ip, src) == 0 && strcmp(curr->dst_ip, dst) == 0) {
            curr->last_seen = time(NULL);
            return curr;
        }
        curr = curr->next;
    }
    
    // 创建新记录
    flow_entry_t *new_flow = (flow_entry_t*)malloc(sizeof(flow_entry_t));
    strcpy(new_flow->src_ip, src);
    strcpy(new_flow->dst_ip, dst);
    new_flow->rx_bytes = 0;
    new_flow->tx_bytes = 0;
    new_flow->rx_packets = 0;
    new_flow->tx_packets = 0;
    new_flow->last_seen = time(NULL);
    new_flow->next = flow_table;
    flow_table = new_flow;
    flow_count++;
    
    return new_flow;
}

// 数据包处理函数
void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet) {
    struct ip *ip_hdr;
    char src_ip[20], dst_ip[20];
    
    // 跳过以太网头(14字节)，定位IP头
    ip_hdr = (struct ip*)(packet + 14);
    
    // 解析IP地址
    inet_ntop(AF_INET, &(ip_hdr->ip_src), src_ip, sizeof(src_ip));
    inet_ntop(AF_INET, &(ip_hdr->ip_dst), dst_ip, sizeof(dst_ip));
    
    // 查找或创建流记录
    flow_entry_t *flow = find_or_create_flow(src_ip, dst_ip);
    
    // 判断方向：源IP以192.168开头认为是内网发出的流量
    if (strncmp(src_ip, "192.168.", 8) == 0) {
        flow->tx_bytes += header->len;
        flow->tx_packets++;
    } else {
        flow->rx_bytes += header->len;
        flow->rx_packets++;
    }
}

// 打印统计信息到控制台
void print_stats() {
    system("clear");
    printf("\n");
    printf("============================================================\n");
    printf("                OpenWrt 流量监控系统                          \n");
    printf("============================================================\n");
    printf("%-18s %-18s %-12s %-8s\n", "源IP", "目的IP", "流量(字节)", "包数");
    printf("------------------------------------------------------------\n");
    
    flow_entry_t *curr = flow_table;
    while (curr) {
        unsigned long total = curr->tx_bytes + curr->rx_bytes;
        if (total > 0) {
            printf("%-18s %-18s %-12lu %-8d\n",
                   curr->src_ip, curr->dst_ip, total, curr->tx_packets + curr->rx_packets);
        }
        curr = curr->next;
    }
    printf("============================================================\n");
    printf("当前活跃连接数: %d\n", flow_count);
    printf("按 Ctrl+C 停止监控\n");
}

int main() {
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    char *dev;
    
    // 1. 查找网络设备
    dev = pcap_lookupdev(errbuf);
    if (dev == NULL) {
        printf("找不到网卡设备: %s\n", errbuf);
        return 1;
    }
    printf("监控网卡: %s\n", dev);
    
    // 2. 打开网络设备（混杂模式）
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        printf("打开网卡失败: %s\n", errbuf);
        return 1;
    }
    
    // 3. 设置过滤器（只捕获IP数据包）
    struct bpf_program filter;
    if (pcap_compile(handle, &filter, "ip", 0, 0) == -1) {
        printf("编译过滤器失败\n");
        return 1;
    }
    if (pcap_setfilter(handle, &filter) == -1) {
        printf("设置过滤器失败\n");
        return 1;
    }
    
    printf("过滤器已设置: ip\n");
    printf("开始捕获数据包...\n\n");
    
    // 4. 循环捕获数据包，每2秒打印一次统计
    time_t last_print = 0;
    while (1) {
        // 非阻塞方式捕获数据包
        pcap_dispatch(handle, 100, packet_handler, NULL);
        
        time_t now = time(NULL);
        if (now - last_print >= 2) {
            print_stats();
            last_print = now;
        }
    }
    
    pcap_close(handle);
    return 0;
}
