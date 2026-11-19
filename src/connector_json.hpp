#pragma once
// Bounded JSON parser for API responses. No eval, unbounded recursion, or external entities.
namespace ConnectorJson {
struct Token {int start,end,next,parent;char type;};
struct Document {
 const char* text;int length,at=0,count=0;Token tokens[4096];bool valid=false;
 Document(const char* value):text(value),length((int)strlen(value)){int root=node(-1,0);space();valid=root==0&&at==length;}
 void space(){while(at<length&&(text[at]==' '||text[at]=='\r'||text[at]=='\n'||text[at]=='\t'))at++;}
 static bool hex(char c){return c>='0'&&c<='9'||c>='a'&&c<='f'||c>='A'&&c<='F';}
 int node(int parent,int depth){space();if(depth>32||count>=4096||at>=length)return -1;int index=count++;Token& token=tokens[index];token={at,at,0,parent,text[at]};char c=text[at++];
  if(c=='"'){token.start=at;while(at<length&&text[at]!='"'){unsigned char p=text[at++];if(p<32)return -1;if(p=='\\'){if(at>=length)return -1;char escaped=text[at++];if(escaped=='u'){for(int i=0;i<4;i++)if(at>=length||!hex(text[at++]))return -1;}else if(!strchr("\"\\/bfnrt",escaped))return -1;}}if(at>=length)return -1;token.end=at++;}
  else if(c=='{'||c=='['){char close=c=='{'?'}':']';space();if(at<length&&text[at]==close){at++;}else{for(;;){if(c=='{'&&(at>=length||text[at]!='"'))return -1;if(node(index,depth+1)<0)return -1;space();if(c=='{'){if(at>=length||text[at++]!=':')return -1;if(node(index,depth+1)<0)return -1;space();}if(at>=length)return -1;if(text[at]==close){at++;break;}if(text[at++]!=',')return -1;space();}}token.end=at;}
  else {while(at<length&&!strchr(",]} \t\r\n",text[at]))at++;token.end=at;int size=token.end-token.start;bool literal=(size==4&&!memcmp(text+token.start,"true",4))||(size==5&&!memcmp(text+token.start,"false",5))||(size==4&&!memcmp(text+token.start,"null",4));if(!literal){int i=token.start;if(text[i]=='-')i++;if(i>=at)return -1;if(text[i]=='0')i++;else{if(text[i]<'1'||text[i]>'9')return -1;while(i<at&&text[i]>='0'&&text[i]<='9')i++;}if(i<at&&text[i]=='.'){i++;int begin=i;while(i<at&&text[i]>='0'&&text[i]<='9')i++;if(i==begin)return -1;}if(i<at&&(text[i]=='e'||text[i]=='E')){i++;if(i<at&&(text[i]=='+'||text[i]=='-'))i++;int begin=i;while(i<at&&text[i]>='0'&&text[i]<='9')i++;if(i==begin)return -1;}if(i!=at)return -1;}}
  token.next=count;return index;
 }
 bool equals(int index,const char* key)const {return index>=0&&index<count&&tokens[index].type=='"'&&tokens[index].end-tokens[index].start==(int)strlen(key)&&!memcmp(text+tokens[index].start,key,strlen(key));}
 int get(int object,const char* key)const {if(!valid||object<0||object>=count||tokens[object].type!='{')return -1;for(int i=object+1;i<tokens[object].next;){int value=i+1;if(equals(i,key))return value;i=tokens[value].next;}return -1;}
 int item(int array,int position)const {if(!valid||array<0||array>=count||tokens[array].type!='[')return -1;int i=array+1;while(position--&&i<tokens[array].next)i=tokens[i].next;return i<tokens[array].next?i:-1;}
 int integer(int index,int fallback=0)const {if(index<0||index>=count)return fallback;return atoi(text+tokens[index].start);}
 bool string(int index,wchar_t* output,int capacity)const {output[0]=0;if(!valid||index<0||index>=count||tokens[index].type!='"')return false;int out=0,i=tokens[index].start,end=tokens[index].end;while(i<end&&out<capacity-1){if(text[i]!='\\'){int begin=i;while(i<end&&text[i]!='\\')i++;int needed=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text+begin,i-begin,0,0);if(!needed&&i>begin)return false;if(out+needed>=capacity){int bytes=0,units=0,remaining=capacity-1-out;while(begin+bytes<i){unsigned char c=text[begin+bytes];int width=c<128?1:c<224?2:c<240?3:4;int n=width==4?2:1;if(units+n>remaining)break;bytes+=width;units+=n;}if(bytes)out+=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text+begin,bytes,output+out,remaining);break;}MultiByteToWideChar(CP_UTF8,0,text+begin,i-begin,output+out,needed);out+=needed;}else{i++;char c=text[i++];if(c=='u'){unsigned value=0;for(int j=0;j<4;j++){char h=text[i++];value=value*16+(h<='9'?h-'0':(h|32)-'a'+10);}output[out++]=(wchar_t)value;}else output[out++]=c=='n'?L'\n':c=='r'?L'\r':c=='t'?L'\t':c=='b'?L'\b':c=='f'?L'\f':c;}}if(out&&output[out-1]>=0xd800&&output[out-1]<=0xdbff)out--;output[out]=0;return true;}
 bool ascii(int index,char* out,int capacity)const {wchar_t value[8192];if(!string(index,value,8192))return false;int bytes=WideCharToMultiByte(CP_UTF8,0,value,-1,out,capacity,0,0);if(!bytes){out[0]=0;return false;}return true;}
};
}
