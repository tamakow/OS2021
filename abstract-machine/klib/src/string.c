#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) { 
  size_t len = 0;
  while(*(s+len)!='\0'){
    ++len;
  }
  return len;
}

char *strcpy(char* dst,const char* src) {  
  char *tmp = dst;
  while(*src!='\0'){
    *tmp++ = *src++;
  }
  return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
  char *tmp = dst;
  while(n>0&&*src!='\0'){
    *tmp++= *src++;
    n--;
  }
  while(n--){
    *tmp++='\0';
  }
  return dst;
}

char* strcat(char* dst, const char* src) { 
  char *tmp= dst;
  while(*tmp!='\0') tmp++;
  while(*src!='\0'){
    *tmp++=*src++;
  }
  *tmp='\0';
  return dst;
}

int strcmp(const char* s1, const char* s2) {
  while(*s1!='\0'&&*s2!='\0'&&*s1==*s2){
    s1++;
    s2++;
  }
  if(*s1 == *s2) return 0;
  else if(*s1 > *s2) return 1;
  else return -1;
}

int strncmp(const char* s1, const char* s2, size_t n) {
  // if strlen(s1) < n && strlen(s2) < n 可能会比较尴尬，只要保证输入合法
  for(size_t i = 0; i < n; ++ i){
    if(*(s1+i) > *(s2+i)) return 1;
    else if(*(s1+i) < *(s2+i)) return -1;
  }
  return 0;
}

void* memset(void* v,int c,size_t n) { 
  void* tmp = v;
  while(n--){
    *(char*)v = (char)c;
    v = (char*)v + 1;
  }
  return tmp;
}

void* memmove(void* dst,const void* src,size_t n) {
  //TODO();
  char* d = (char*) dst;
  const char* s = (const char*) src;
  if(s>d){
    while(n--){
      *d++ =*s++;
    }
  }
  else if(s<d){
    d = d+n-1;
    s = s+n-1;
    while(n--){
      *d-- = *s--;
    }
  }
  return dst;
}

void* memcpy(void* out, const void* in, size_t n) {
  char *charout = (char *)out;
  char *charin = (char *)in;
  while(n--){
    *charout++ = *charin++;
  }
  return out;
}

int memcmp(const void* s1, const void* s2, size_t n) { 
  char* chars1 = (char*)s1;
  char* chars2 = (char*)s2;
  for(size_t i = 0; i < n; ++i){
    if(*(chars1+i) > *(chars2+i)) return 1;
    else if(*(chars1+i) < *(chars2+i)) return -1;
  }
  return 0;
}

#endif
