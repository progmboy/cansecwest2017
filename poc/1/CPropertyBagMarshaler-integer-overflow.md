# CPropertyBagMarshaler integer overflow

## Author
pgboy1988(360Vulcan Team)

## Environment

Win10 14939 rs1 32bit all patch.

## Root Case
We found this bug in 'DirectComposition::CPropertyBagMarshaler::UpdatePropertyValue' function.
```c
unsigned int 
__thiscall 
DirectComposition::CPropertyBagMarshaler::UpdatePropertyValue(
	CPropertyBagMarshalerStruct *this, 
	BAGMARSHALERINPUT *InBuffer, 
	unsigned int dwSize);
```

This function call from DirectComposition::CPropertyBagMarshaler::SetBufferProperty
see pseudo code

```c
NTSTATUS __thiscall DirectComposition::CPropertyBagMarshaler::SetBufferProperty(CPropertyBagMarshalerStruct *this, struct DirectComposition::CApplicationChannel *a2, unsigned int dwProperty, const void *pInBuffer, size_t dwSize, bool *a6)
{
  signed int v6; // esi@1
  signed int v8; // eax@4
  void *newBuffer; // ecx@10 MAPDST
  int oldSize_; // edx@11
  int newBuffer_; // edi@12
  int oldSize; // [sp+14h] [bp-6Ch]@11
  int oldSizea; // [sp+14h] [bp-6Ch]@13
  int v16; // [sp+1Ch] [bp-64h]@18
  int v17; // [sp+20h] [bp-60h]@18
  int v18; // [sp+24h] [bp-5Ch]@18
  int v19; // [sp+28h] [bp-58h]@18
  char copyedBuffer; // [sp+2Ch] [bp-54h]@4

  v6 = 0;
  if ( dwProperty ){
    if ( dwProperty == 1 ){
      if ( dwSize >= 0x10 ){
        memcpy(&copyedBuffer, pInBuffer, dwSize); //other bug. stack overflow
        
        //
        // here call DirectComposition::CPropertyBagMarshaler::UpdatePropertyValue
        //
        
        v8 = DirectComposition::CPropertyBagMarshaler::UpdatePropertyValue(
               this,
               (BAGMARSHALERINPUT *)&copyedBuffer,
               dwSize);
        goto LABEL_5;
      }
    }else if ( dwProperty == 2 ){
      newBuffer = (void *)Win32AllocPoolWithQuota(dwSize, 'xpCD');
      if ( !newBuffer )
        return STATUS_NO_MEMORY;
      oldSize = this->DataSize;
      oldSize_ = this->DataSize;
      this->DataSize = dwSize;
      if ( dwSize < oldSize_ + 0x10 ){
        memcpy(newBuffer, pInBuffer, dwSize);
        newBuffer_ = (int)newBuffer;
      }else{
        memcpy(newBuffer, (const void *)this->DataBuffer, oldSize_);
        newBuffer_ = (int)newBuffer;
        memcpy((char *)newBuffer + oldSize, (char *)pInBuffer + oldSize, this->DataSize - oldSize);
      }
      oldSizea = this->DataBuffer;
      if ( oldSizea && IsWin32FreePoolImplSupported() >= 0 )
        Win32FreePoolImpl(oldSizea);
      this->DataBuffer = newBuffer_;
      goto LABEL_7;
    }
    return 0xC000000D;
  }
	...
  return v6;
}
```
In the branch 'dwProperty == 2 ' we can set 'this->DataSize' and 'this->DataBuffer'. by default 'this->DataSize' is 0, and 'this->DataBuffer' is NULL. remember

let's look at the pseudo code of DirectComposition::CPropertyBagMarshaler::UpdatePropertyValue

```c
NTSTATUS __thiscall DirectComposition::CPropertyBagMarshaler::UpdatePropertyValue(CPropertyBagMarshalerStruct *this, BAGMARSHALERINPUT *InBuffer, unsigned int dwSize)
{
  unsigned int Offsets; // esi@1
  int Type; // eax@2
  int v6; // edi@2
  int v7; // eax@4
  int v8; // eax@5
  int v9; // eax@6
  int v11; // eax@13
  int v12; // eax@14
  int v13; // eax@22
  char *v14; // esi@25
  _DWORD *v15; // edi@25
  int v16; // ecx@27
  int *v17; // edi@31
  int v18; // edi@32
  int v19; // esi@32
  signed int v20; // [sp-8h] [bp-10h]@8

  Offsets = InBuffer->Offsets;
  
  //
  // Integer overflow
  //
  
  if ( Offsets < this->DataSize - 0xC ){
    Type = InBuffer->Type;
    v6 = Offsets + this->DataBuffer;
    if ( *(_DWORD *)v6 != Type )
      return STATUS_INVALID_PARAMETER;
    if ( Type <= 0x45 ){
      if ( Type != 0x45 ){
        v11 = Type - 0x11;
        if ( v11 ){
          v12 = v11 - 1;
          if ( v12 ){
            v13 = v12 - 0x11;
            if ( v13 ){
              if ( v13 == 0x11 && dwSize == 0x1C ){
                v14 = (char *)&InBuffer->field_16;
                v15 = (_DWORD *)(v6 + 0xC);
LABEL_32:
                *v15 = *(_DWORD *)v14;
                v19 = (int)(v14 + 4);
                v18 = (int)(v15 + 1);
                //write two dword
                *(_DWORD *)v18 = *(_DWORD *)v19;
                *(_DWORD *)(v18 + 4) = *(_DWORD *)(v19 + 4);
                return DirectComposition::CPropertyBagMarshaler::AddPropertyUpdate(
                         this,
                         (const struct PropertyUpdate *)InBuffer);
              }
            }else if ( dwSize == 0x18 ){
              v16 = InBuffer->field_20;
              //write two dword
              *(_DWORD *)(v6 + 0xC) = InBuffer->field_16;
              *(_DWORD *)(v6 + 0x10) = v16;
              return DirectComposition::CPropertyBagMarshaler::AddPropertyUpdate(
                       this,
                       (const struct PropertyUpdate *)InBuffer);
            }
          }else if ( dwSize == 0x14 ){
            //write float
            *(float *)(v6 + 0xC) = *(float *)&InBuffer->field_16;
            return DirectComposition::CPropertyBagMarshaler::AddPropertyUpdate(
                     this,
                     (const struct PropertyUpdate *)InBuffer);
          }
        }else if ( dwSize == 0x14 ){
          //write bytes
          *(_BYTE *)(v6 + 0xC) = InBuffer->field_16;
          return DirectComposition::CPropertyBagMarshaler::AddPropertyUpdate(
                   this,
                   (const struct PropertyUpdate *)InBuffer);
        }
        return STATUS_INVALID_PARAMETER;
      }
    }else{
      v7 = Type - 0x46;
      if ( v7 ){
        v8 = v7 - 1;
        if ( v8 ){
          v9 = v8 - 0x21;
          if ( v9 ){
            if ( v9 == 0xA1 && dwSize == 0x50 ){
              v20 = 0x10;
              goto LABEL_9;
            }
          }else if ( dwSize == 0x28 ){
            v20 = 6;
LABEL_9:
			// write 24 bytes
            qmemcpy((void *)(v6 + 0xC), &InBuffer->field_16, 4 * v20);
            return DirectComposition::CPropertyBagMarshaler::AddPropertyUpdate(
                     this,
                     (const struct PropertyUpdate *)InBuffer);
          }
          return STATUS_INVALID_PARAMETER;
        }
      }
    }
    if ( dwSize == 0x20 ){
      v17 = (int *)(v6 + 12);
      *v17 = InBuffer->field_16;
      v14 = (char *)&InBuffer->field_20;
      v15 = v17 + 1;
      goto LABEL_32;
    }
    return STATUS_INVALID_PARAMETER;
  }
  return 0xC000000D;
}
```
The InBuffer Parameter is pass by usermode, we can directly control it.
See the branch 'if ( Offsets < this->DataSize - 0xC )', by default this->DataSize is 0, 'this->DataSize - 0xc' is 0xFFFFFFF4, this is a integer overflow. And we can also set this->DataSize to a value which less than 0xc. 
You can see that allmost all branches have one or more write operation which write something to v6+[offset].
And "v6 = Offsets + this->DataBuffer" the 'offsets' we can control. by default 'this->DataBuffer' is NULL if you do not do anything else. In 32Bits operating system we can total control all the address space. But in 64 bits windows it can only control low 32bits memory. so in 64bits windows we must find a path which could control 'this->DataBuffer'.
In function 'DirectComposition::CPropertyBagMarshaler::SetBufferProperty' we can set 'this->DataBuffer'.

SO we can overflow 'this->DataBuffer' write something.
