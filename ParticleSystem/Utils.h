#pragma once
#define SafeRelease(x) if(x)x->Release();x=nullptr