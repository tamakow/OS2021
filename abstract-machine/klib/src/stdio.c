#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

char* zx_itoa(int num,char* str,int radix){ 
  if(radix < 2 || (num<0 && radix != 10)){ //set radix 10 normally
    return NULL;
  }
  if(num == 0){
    *str = '0';
    *(str+1) = '\0';
    return str;
  }

  char *p =str;
  int src=num;
  num = (src<0)?num*(-1):num;
  while(num!=0){
    if(num%radix <10){
      *p = num%radix + '0';
    }
    else{
      *p = num%radix - 10 + 'a';
    }
    num = num/radix;
    p++;
  }
  if(src<0){
    *p = '-';
    p++;
  }
  *p='\0';
  for(char* left =str,*right = p-1;left<right;left++,right--){
    char temp = *left;
    *left = *right;
    *right =temp;
  }
  return str;
}

//printf 的 %p %0 有问题 

int printf(const char *fmt, ...) {
  int len = strlen(fmt);
  int pre=0;
  int return_val=0;
  va_list Args;
  va_start(Args, fmt);
  while(pre<len){
    if(fmt[pre] != '%'){
      putch(fmt[pre++]);
      return_val++;
    }
    else{
      pre++;
      if(fmt[pre]== 's'){
        char *tmp = va_arg(Args,char*);
        for(int i=0;i<strlen(tmp);++i) putch(tmp[i]);
        return_val += strlen(tmp);
      }
      else if(fmt[pre] == 'c'){
        char tmp = va_arg(Args,int);
        putch(tmp);
        return_val+=1;
      }
      else if(fmt[pre] == 'd'){
        int tmp_num = va_arg(Args,int);
        char tmp[128]="";
        zx_itoa(tmp_num,tmp,10);
        for(int i=0;i<strlen(tmp);++i) putch(tmp[i]);
        return_val += strlen(tmp);
      }
      else if(fmt[pre] == 'u'){
        unsigned tmp_num = va_arg(Args,unsigned);
        char tmp[128]="";
        int j=0;
        // zx_itoa(tmp_num,tmp,10);
        do{
          int tmp_1 = tmp_num % 10;
          tmp[j++] = tmp_1 + '0';
          tmp_num/=10;
        }while(tmp_num);
        for(int i= j-1;i>=0;--i) putch(tmp[i]);
        return_val += strlen(tmp);
      }
      else if(fmt[pre] == 'x' || fmt[pre] == 'p'){ //这里p是不对的，因为应该是指针地址
        unsigned tmp_num = va_arg(Args,unsigned);
        char tmp[128]="";
        zx_itoa(tmp_num,tmp,16);
        for(int i=0;i<strlen(tmp);++i) putch(tmp[i]);
        return_val += strlen(tmp);
      }
      else if(fmt[pre] == '%'){
        putch(fmt[pre]);
        putch(fmt[++pre]);
        return_val+=2;
      }
      else if(fmt[pre] == '0'){ //只为了时钟里的%02d
        int j=fmt[++pre] - '0';
        int tmp_num = va_arg(Args,int);
        char tmp[128]="";
        zx_itoa(tmp_num,tmp,10);
        for(int i= strlen(tmp);i<j;++i){
          return_val++;
          putch('0');
        }
        for(int i=0;i<strlen(tmp);++i) putch(tmp[i]);
        return_val += strlen(tmp);
        pre++;
      }
      else {
        printf("TO BE DONE\n");
        printf("%c\n",fmt[pre]);
        assert(0);
      }
      pre++;
    }
  }
  va_end(Args);
  return return_val;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return 0;
}



int sprintf(char *out, const char *fmt, ...) {
  int len = strlen(fmt);
  int pre=0;
  int return_val=0;
  *out = '\0';
  va_list Args;
  va_start(Args, fmt);
  while(pre<len){
    if(fmt[pre] != '%'){
      char tmp[2];
      tmp[0]=fmt[pre];
      tmp[1]='\0';
      strcat(out,tmp);
      pre++;
      return_val++;
    }
    else{
      pre++;
      if(fmt[pre]== 's'){
        char *tmp = va_arg(Args,char*);
        strcat(out,tmp);
        return_val += strlen(tmp);
      }
      else if(fmt[pre] == 'd'){
        int tmp_num = va_arg(Args,int);
        char tmp[128]="";
        zx_itoa(tmp_num,tmp,10);
        strcat(out,tmp);
        return_val += strlen(tmp);
      }
      else {
        printf("to be continue\n");
        assert(0);
      }
      pre++;
    }
  }
  va_end(Args);
  return return_val;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  return 0;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  return 0;
}

#endif
