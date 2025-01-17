#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "bits.bpf.h"
#include "maps.bpf.h"

// 17 buckets, max range is 32k..64k
#define MAX_BUCKET_SLOT 17

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, MAX_BUCKET_SLOT + 2);
    __type(key, u64);
    __type(value, u64);
} tcp_syn_backlog SEC(".maps");

static int do_count(u64 backlog)
{
    u64 bucket;

    // Histogram key for exp2zero
    if (backlog == 0) {
        bucket = 0;
    } else {
        bucket = log2l(backlog) + 1;
    }

    if (bucket > MAX_BUCKET_SLOT) {
        bucket = MAX_BUCKET_SLOT;
    }

    increment_map(&tcp_syn_backlog, &bucket, 1);

    // No use incrementing by zero
    if (backlog > 0) {
        bucket = MAX_BUCKET_SLOT + 1;
        increment_map(&tcp_syn_backlog, &bucket, backlog);
    }

    return 0;
}

SEC("kprobe/tcp_v4_syn_recv_sock")
int BPF_KPROBE(kprobe__tcp_v4_syn_recv_sock, struct sock *sk)
{
    return do_count(BPF_CORE_READ(sk, sk_ack_backlog) / 50);
}

SEC("kprobe/tcp_v6_syn_recv_sock")
int BPF_KPROBE(kprobe__tcp_v6_syn_recv_sock, struct sock *sk)
{
    return do_count(BPF_CORE_READ(sk, sk_ack_backlog) / 50);
}

char LICENSE[] SEC("license") = "GPL";
