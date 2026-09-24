#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/init.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
#include <syscalls/syscalls.h>
#include <stdio.h>
#include <string.h>
#include "libc/memmgr.h"

#define SW 528
#define SH 320
#define PVR (*(volatile unsigned long *)0xff000030)
#define PRR (*(volatile unsigned long *)0xff000044)
#define FRQCR (*(volatile unsigned long *)0xa4150000)
#define RTC_SEC (*(volatile unsigned char *)0xa413fec2)
#define TMU_TSTR (*(volatile unsigned char *)0xa4490004)
#define TMU2_TCOR (*(volatile unsigned long *)0xa4490020)
#define TMU2_TCNT (*(volatile unsigned long *)0xa4490024)
#define TMU2_TCR (*(volatile unsigned short *)0xa4490028)

typedef struct {char sig[12],model[4];unsigned long magic,nor_size;} RomHeader;
typedef struct {
 unsigned long pvr,prr,frqcr,nor_size,pclk,iclk,bclk,shclk,free_kib;
 unsigned int pll,idiv,bdiv,pdiv,shdiv,save_ok;
 char model[5],dump[2048];
} Probe;

static unsigned int divisor(unsigned int c){static const unsigned char d[16]={2,3,4,6,8,12,16,0,24,32,36,48,0,72,0,0};return d[c&15];}
static void release(int k){while(get_key_state(k))keypad_read();}
static void clear(void){set_pen(create_rgb16(0,0,0));draw_rect(0,0,SW,SH);}
static void line(int y,const char *s,unsigned short color){set_pen(color);render_text(12,y,(char*)s);}
static unsigned long measure_pclk(void){
 unsigned char old_tstr=TMU_TSTR,s0,s1;unsigned short old_tcr=TMU2_TCR;unsigned long old_tcor=TMU2_TCOR,old_tcnt=TMU2_TCNT,a,b,guard=0;
 TMU_TSTR=(unsigned char)(old_tstr&~4);TMU2_TCOR=0xffffffffUL;TMU2_TCNT=0xffffffffUL;TMU2_TCR=0;TMU_TSTR=(unsigned char)((old_tstr&~4)|4);
 s0=RTC_SEC;while((s1=RTC_SEC)==s0&&++guard<200000000UL){}if(guard>=200000000UL)goto fail;a=TMU2_TCNT;guard=0;s0=s1;while((s1=RTC_SEC)==s0&&++guard<200000000UL){}if(guard>=200000000UL)goto fail;b=TMU2_TCNT;
 TMU_TSTR=(unsigned char)(TMU_TSTR&~4);TMU2_TCOR=old_tcor;TMU2_TCNT=old_tcnt;TMU2_TCR=old_tcr;TMU_TSTR=old_tstr;return (a-b)*4UL;
fail: TMU_TSTR=(unsigned char)(TMU_TSTR&~4);TMU2_TCOR=old_tcor;TMU2_TCNT=old_tcnt;TMU2_TCR=old_tcr;TMU_TSTR=old_tstr;return 0;
}
static unsigned long probe_memory(void){void *p[32],*large;unsigned int n=0,i;while(n<32&&(p[n]=memmgr_alloc(256UL*1024UL))!=0){((unsigned char*)p[n])[0]=0x5a;((unsigned char*)p[n])[256UL*1024UL-1]=0xa5;n++;}for(i=n;i>0;i--)memmgr_free(p[i-1]);if(n<2)return n*256UL;large=memmgr_alloc((n-1)*256UL*1024UL);if(!large)return 0;((unsigned char*)large)[0]=0x3c;((unsigned char*)large)[(n-1)*256UL*1024UL-1]=0xc3;memmgr_free(large);return (n-1)*256UL;}
static void build_dump(Probe *p){char *q=p->dump;unsigned long drive;char id[10],path[64];int fd,n;
 q+=sprintf(q,"EX-WORD HARDWARE DUMP v1\r\n");q+=sprintf(q,"ROM_MODEL=%s\r\nNOR_BYTES=%u\r\n",p->model,(unsigned int)p->nor_size);
 q+=sprintf(q,"PVR=%08x\r\nPRR=%08x\r\nFRQCR=%08x\r\n",(unsigned int)p->pvr,(unsigned int)p->prr,(unsigned int)p->frqcr);
 q+=sprintf(q,"PLL_MULT=%u\r\nDIV_I=%u\r\nDIV_SH=%u\r\nDIV_B=%u\r\nDIV_P=%u\r\n",p->pll,p->idiv,p->shdiv,p->bdiv,p->pdiv);
 q+=sprintf(q,"ICLK_HZ=%u\r\nSHCLK_HZ=%u\r\nBCLK_HZ=%u\r\nPCLK_HZ=%u\r\n",(unsigned int)p->iclk,(unsigned int)p->shclk,(unsigned int)p->bclk,(unsigned int)p->pclk);
 q+=sprintf(q,"CPU_CORES=1 (architecture)\r\nAPP_HEAP_CONTIG_KIB=%u\r\nFRAMEBUFFER_BYTES=337920\r\nDISPLAY=528x320 RGB565\r\n",(unsigned int)p->free_kib);
 q+=sprintf(q,"GPU=not identified\r\nNPU=not present in SH-4A ISA\r\nIDLE_POWER_MW=external measurement required\r\n");
 sys_dict_info(&drive,id);sprintf(path,"\\\\drv0\\%s\\_USER\\HWINFO.TXT",id);if(sys_create(path,1)<0){p->save_ok=0;return;}fd=sys_open(path,FILE_WR);if(fd<0){p->save_ok=0;return;}n=(int)(q-p->dump);p->save_ok=sys_write(fd,p->dump,n)==n;sys_close(fd);
}
static void collect(Probe *p){RomHeader *r=(RomHeader*)0x8001ff80;memset(p,0,sizeof(*p));p->pvr=PVR;p->prr=PRR;p->frqcr=FRQCR;if(!memcmp(r->sig,"CASIODICS",9)){memcpy(p->model,r->model,4);p->model[4]=0;p->nor_size=r->nor_size;}else strcpy(p->model,"????");p->pll=(((p->frqcr>>24)&63)+1)*2;p->idiv=divisor(p->frqcr>>20);p->shdiv=divisor(p->frqcr>>12);p->bdiv=divisor(p->frqcr>>8);p->pdiv=divisor(p->frqcr);p->pclk=measure_pclk();if(p->pclk&&p->idiv&&p->pdiv)p->iclk=(p->pclk/p->idiv)*p->pdiv;if(p->pclk&&p->bdiv&&p->pdiv)p->bclk=(p->pclk/p->bdiv)*p->pdiv;if(p->pclk&&p->shdiv&&p->pdiv)p->shclk=(p->pclk/p->shdiv)*p->pdiv;p->free_kib=probe_memory();build_dump(p);}
static void mhz(char *s,const char *name,unsigned long hz){if(hz)sprintf(s,"%s: %u.%03u MHz",name,(unsigned int)(hz/1000000UL),(unsigned int)((hz/1000UL)%1000UL));else sprintf(s,"%s: measurement failed",name);}
static void draw_page(Probe *p,unsigned int page){char s[96];clear();line(10,"EX-WORD HARDWARE DUMP",create_rgb16(0,255,255));if(page==0){line(42,"CPU / CLOCK",create_rgb16(255,255,0));sprintf(s,"ROM model: %s   cores: 1",p->model);line(72,s,create_rgb16(255,255,255));mhz(s,"CPU I-clock",p->iclk);line(102,s,create_rgb16(255,255,255));mhz(s,"Bus B-clock",p->bclk);line(132,s,create_rgb16(255,255,255));mhz(s,"Peripheral P-clock",p->pclk);line(162,s,create_rgb16(255,255,255));sprintf(s,"PLL x%u  div I/B/P %u/%u/%u",p->pll,p->idiv,p->bdiv,p->pdiv);line(192,s,create_rgb16(180,180,180));}
 else if(page==1){line(42,"RAW IDENTIFICATION",create_rgb16(255,255,0));sprintf(s,"PVR   %08x",(unsigned int)p->pvr);line(72,s,create_rgb16(255,255,255));sprintf(s,"PRR   %08x",(unsigned int)p->prr);line(102,s,create_rgb16(255,255,255));sprintf(s,"FRQCR %08x",(unsigned int)p->frqcr);line(132,s,create_rgb16(255,255,255));sprintf(s,"NOR header: %u bytes",(unsigned int)p->nor_size);line(162,s,create_rgb16(255,255,255));line(202,"Registers are read-only in this tool.",create_rgb16(180,180,180));}
 else if(page==2){line(42,"MEMORY / DISPLAY",create_rgb16(255,255,0));sprintf(s,"Contiguous app heap: %u KiB",(unsigned int)p->free_kib);line(72,s,create_rgb16(255,255,255));line(102,"Framebuffer: 337920 bytes",create_rgb16(255,255,255));line(132,"Display: 528x320 RGB565",create_rgb16(255,255,255));line(162,"GPU: not identified",create_rgb16(180,180,180));line(192,"NPU: not present in SH-4A ISA",create_rgb16(180,180,180));line(222,"Idle power: needs external meter",create_rgb16(180,180,180));}
 else{line(42,"DUMP FILE",create_rgb16(255,255,0));line(72,p->save_ok?"HWINFO.TXT saved":"HWINFO.TXT save failed",p->save_ok?create_rgb16(0,255,0):create_rgb16(255,0,0));line(112,"Internal: HWDMP/_USER",create_rgb16(255,255,255));line(152,"USB retrieval supported",create_rgb16(255,255,255));line(202,"Storage capacity is measured by host.",create_rgb16(180,180,180));}
 line(290,"LEFT/RIGHT: page   BACK: exit",create_rgb16(0,255,0));lcdc_copy_vram();}
int main(void *ptr){Probe p;unsigned int page=0;if(ptr&&*(long*)ptr==1)return -1;memmgr_init();graphics_init(SW,SH,(void*)0xAC200000);clear();line(120,"Measuring hardware...",create_rgb16(255,255,0));lcdc_copy_vram();collect(&p);draw_page(&p,page);for(;;){keypad_read();if(get_key_state(KEY_POWER)||get_key_state(KEY_BACK))return -2;if(get_key_state(KEY_LEFT)){release(KEY_LEFT);page=(page+3)%4;draw_page(&p,page);}if(get_key_state(KEY_RIGHT)||get_key_state(KEY_ENTER)){if(get_key_state(KEY_RIGHT))release(KEY_RIGHT);if(get_key_state(KEY_ENTER))release(KEY_ENTER);page=(page+1)%4;draw_page(&p,page);}}}
