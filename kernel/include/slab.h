#ifndef __SLAB_H__
#define __SLAB_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      (4 KiB)
#define  NR_PAGE_CACHE  4          //没事就多分点 
#define  FLAG_SIZE      (sizeof(bool))

/*
 *   |-------------------------------------------------------heap----------------------------------------------------------------------------|
 *   |---page---|---page---|--- *** ---|---page---|---page---|---kmem_cache---|--- *** ---|---kmem_cache---|---flag---|--- *** ---|---flag---|
 *   |-----------------------nr_pages个----------------------|---------------nr_pages_cache个---------------|-----------nr_pages个------------|
 *   |----------------------用于分配的页面---------------------|-----------------快速分配内存-------------------|--------判断对应的页面是否空闲------|
 */

struct item {
    bool used;               //简单的判断是否已经被分配
    struct item *next;
};


struct slab {
    size_t item_size;        // slab 的每个item的大小
    int max_item_nr;         // 最多可以有的 item数量
    int now_item_nr;         // 现在有的item数量
    int slab_alloc_pages;    // 需要分配的页面数目
    void *st;                // start pointer
    struct item *items;      // slab的item
    struct slab *next;       // 相同size的slab
};

//将kmem_cache[MAX_CPU + 1]改成struct, 添加一下slab_free，slab_partial和slab_full,并添加item_size显示slab的size ，还有总共分配的页面数
//https://segmentfault.com/a/1190000022506020
struct kmem_cache {
    int cpu;                 // 所属的cpu
    struct spinlock lock;    // 每个的锁
    size_t slab_item_size;   // 和slab中的一样
    int slab_max_item_nr;    // slab中最多的item数量     
    int cache_alloc_pages;   
    struct slab* slabs_partial;
    struct slab* slabs_full;
    struct slab* slabs_free;
};


#endif