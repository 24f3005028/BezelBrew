#pragma once

struct ToolbarLayout {
    static constexpr int count=12;
    static constexpr unsigned all=(1u<<count)-1;
    int order[count];
    unsigned shown;

    bool visible(int tile) const {return (shown&(1u<<tile))!=0;}
    int visibleCount() const {
        int total=0;
        for(int tile:order)if(visible(tile))total++;
        return total;
    }
    static ToolbarLayout defaults(){
        ToolbarLayout layout={};
        for(int i=0;i<count;i++)layout.order[i]=i;
        layout.shown=all;
        return layout;
    }
    void load(const wchar_t* file){
        wchar_t text[64]={};
        GetPrivateProfileString(L"TopBar",L"Order",L"",text,ARRAYSIZE(text),file);
        const wchar_t* next=text;
        unsigned used=0;
        int position=0;
        while(*next&&position<count){
            wchar_t* end=nullptr;
            long tile=wcstol(next,&end,10);
            if(end==next||(*end!=L','&&*end))break;
            if(tile>=0&&tile<count&&!(used&(1u<<tile))){
                order[position++]=(int)tile;
                used|=1u<<tile;
            }
            next=*end?end+1:end;
        }
        for(int tile=0;tile<count;tile++)if(!(used&(1u<<tile)))order[position++]=tile;
        shown=GetPrivateProfileInt(L"TopBar",L"Visible",all,file)&all;
    }
    bool save(const wchar_t* file) const {
        wchar_t section[112]={};
        int used=swprintf_s(section,L"Visible=%u",shown)+1;
        used+=swprintf_s(section+used,ARRAYSIZE(section)-used,L"Order=");
        for(int i=0;i<count;i++)
            used+=swprintf_s(section+used,ARRAYSIZE(section)-used,L"%s%d",i?L",":L"",order[i]);
        return WritePrivateProfileSection(L"TopBar",section,file)!=FALSE;
    }
};
