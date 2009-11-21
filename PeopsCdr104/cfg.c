/***************************************************************************
                            cfg.c  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#define _IN_CFG
#include "externals.h"

////////////////////////////////////////////////////////////////////////

// msg to update gauge on subdata extract
#define WM_SUBSTART (WM_USER+432)
                        
/////////////////////////////////////////////////////////////////////////////
// read config from registry

void ReadConfig(void)
{
 HKEY myKey;DWORD temp,type,size;

 // init values

 iCD_AD=-1;            
 iCD_TA=-1;               
 iCD_LU=-1;
 iRType=0;
 iUseSpeedLimit=0;
 iSpeedLimit=2;
 iNoWait=0;
 iMaxRetry=5;
 iShowReadErr=0;
 iUsePPF=0;
 iUseSubReading=0;
 iUseDataCache=0;
 memset(szPPF,0,260);
 memset(szSUBF,0,260);

 // read values
 
 if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\CDR\\PeopsCDRASPI",0,KEY_ALL_ACCESS,&myKey)==ERROR_SUCCESS)
  {
   size = 4;
   if(RegQueryValueEx(myKey,"Adapter",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCD_AD=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"Target",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCD_TA=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"LUN",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCD_LU=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"RType",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iRType=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseCaching",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseCaching=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseDataCache",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseDataCache=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseSpeedLimit",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseSpeedLimit=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"SpeedLimit",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iSpeedLimit=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"NoWait",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iNoWait=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"MaxRetry",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iMaxRetry=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"ShowReadErr",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iShowReadErr=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UsePPF",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUsePPF=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseSubReading",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseSubReading=(int)temp;
   size=259;
   RegQueryValueEx(myKey,"PPFFile",0,&type,(LPBYTE)szPPF,&size);
   size=259;
   RegQueryValueEx(myKey,"SCFile",0,&type,(LPBYTE)szSUBF,&size);

   RegCloseKey(myKey);
  }
}

////////////////////////////////////////////////////////////////////////
// write user config

void WriteConfig(void)
{
 HKEY myKey;DWORD myDisp,temp;

 RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\CDR\\PeopsCDRASPI",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&myDisp);
 temp=iInterfaceMode;
 RegSetValueEx(myKey,"InterfaceMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCD_AD;
 RegSetValueEx(myKey,"Adapter",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCD_TA;
 RegSetValueEx(myKey,"Target",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCD_LU;
 RegSetValueEx(myKey,"LUN",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iRType;
 RegSetValueEx(myKey,"RType",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseCaching;
 RegSetValueEx(myKey,"UseCaching",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseDataCache;
 RegSetValueEx(myKey,"UseDataCache",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseSpeedLimit;
 RegSetValueEx(myKey,"UseSpeedLimit",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iSpeedLimit;
 RegSetValueEx(myKey,"SpeedLimit",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iNoWait;
 RegSetValueEx(myKey,"NoWait",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iMaxRetry;
 RegSetValueEx(myKey,"MaxRetry",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iShowReadErr;
 RegSetValueEx(myKey,"ShowReadErr",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUsePPF;
 RegSetValueEx(myKey,"UsePPF",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseSubReading;
 RegSetValueEx(myKey,"UseSubReading",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));

 RegSetValueEx(myKey,"PPFFile",0,REG_BINARY,(LPBYTE)szPPF,259);  
 RegSetValueEx(myKey,"SCFile",0,REG_BINARY,(LPBYTE)szSUBF,259);  

 RegCloseKey(myKey);
}

////////////////////////////////////////////////////////////////////////
// choose ppf/sbi/m3s file name

void OnChooseFile(HWND hW,int iFType)
{
 OPENFILENAME ofn;char szB[260];BOOL b;

 ofn.lStructSize=sizeof(OPENFILENAME); 
 ofn.hwndOwner=hW;                      
 ofn.hInstance=NULL; 
 if(iFType==0)      ofn.lpstrFilter="PPF Files\0*.PPF\0\0\0"; 
 else if(iFType==1) ofn.lpstrFilter="SBI Files\0*.SBI\0M3S Files\0*.M3S\0\0\0"; 
 else if(iFType==2) ofn.lpstrFilter="SUB Files\0*.SUB\0\0\0"; 
 else if(iFType==3) ofn.lpstrFilter="SBI Files\0*.SBI\0\0\0"; 
 else               ofn.lpstrFilter="M3S Files\0*.M3S\0\0\0"; 

 ofn.lpstrCustomFilter=NULL; 
 ofn.nMaxCustFilter=0;
 ofn.nFilterIndex=0; 
 if(iFType==0)      GetDlgItemText(hW,IDC_PPFFILE,szB,259);
 else if(iFType==1) GetDlgItemText(hW,IDC_SUBFILE,szB,259);
 else if(iFType==2) GetDlgItemText(hW,IDC_SUBFILEEDIT,szB,259);
 else if(iFType==3) GetDlgItemText(hW,IDC_OUTFILEEDIT,szB,259);
 else               GetDlgItemText(hW,IDC_OUTFILEEDIT,szB,259);

 ofn.lpstrFile=szB;
 ofn.nMaxFile=259; 
 ofn.lpstrFileTitle=NULL;
 ofn.nMaxFileTitle=0; 
 ofn.lpstrInitialDir=NULL;
 ofn.lpstrTitle=NULL; 
 if(iFType<3)
  ofn.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_HIDEREADONLY;     
 else
  ofn.Flags=OFN_CREATEPROMPT|OFN_NOCHANGEDIR|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;    
 ofn.nFileOffset=0; 
 ofn.nFileExtension=0;
 ofn.lpstrDefExt=0; 
 ofn.lCustData=0;
 ofn.lpfnHook=NULL; 
 ofn.lpTemplateName=NULL;
    
 if(iFType<3)
      b=GetOpenFileName(&ofn);
 else b=GetSaveFileName(&ofn);
                                                  
 if(b)
  {
   if(iFType==0)      SetDlgItemText(hW,IDC_PPFFILE,szB);
   else if(iFType==1) SetDlgItemText(hW,IDC_SUBFILE,szB);
   else if(iFType==2) SetDlgItemText(hW,IDC_SUBFILEEDIT,szB);
   else if(iFType==3) SetDlgItemText(hW,IDC_OUTFILEEDIT,szB);
   else               SetDlgItemText(hW,IDC_OUTFILEEDIT,szB);
  }
}

////////////////////////////////////////////////////////////////////////
// file drive combo

void EnumDrives(HWND hW)
{
 HWND hWC;char szB[256];int i=0,k=0,iNum;
 char * p, * pBuf, * pN;

 hWC=GetDlgItem(hW,IDC_DRIVE);
 ComboBox_ResetContent(hWC);
 ComboBox_AddString(hWC,"NONE");                       // add always existing 'none'

 wsprintf(szB,"[%d:%d:%d",iCD_AD,iCD_TA,iCD_LU);       // make current user info text

 pN=pBuf=(char *)malloc(32768);
 memset(pBuf,0,32768);
 iNum=GetGenCDDrives(pBuf);                            // get the system cd drives list

 for(i=0;i<iNum;i++)                                   // loop drives
  {
   ComboBox_AddString(hWC,pN);                         // -> add drive name
   p=strchr(pN,']');
   if(p)
    {
     *p=0;
     if(strcmp(szB,pN)==0) k=i+1;                      // -> is it the current user drive? sel it
     *p=']';
    }
   pN+=strlen(pN)+1;                                   // next drive in buffer
  }

 free(pBuf);

 ComboBox_SetCurSel(hWC,k);                            // do the drive sel

 hWC=GetDlgItem(hW,IDC_RTYPE);                         // depending on interface: hide certain controls
 if(iInterfaceMode==3)
  {
   ShowWindow(hWC,SW_HIDE);
   ShowWindow(GetDlgItem(hW,IDC_AUTO),SW_HIDE);
   ShowWindow(GetDlgItem(hW,IDC_RAWTXT),SW_SHOW);
  }
 else
  {
   ShowWindow(hWC,SW_SHOW);
   ShowWindow(GetDlgItem(hW,IDC_AUTO),SW_SHOW);
   ShowWindow(GetDlgItem(hW,IDC_RAWTXT),SW_HIDE);
  }
}

////////////////////////////////////////////////////////////////////////
// get curr selected drive

void GetCDRInfos(HWND hW,int * iA, int * iT,int * iL)
{
 HWND hWC=GetDlgItem(hW,IDC_DRIVE);
 char szB[256];int i;char * p;

 i=ComboBox_GetCurSel(hWC);
 if(i<=0)                                              // none selected
  {
   *iA=-1;*iT=-1;*iL=-1;
   MessageBox(hW,"Please select a cdrom drive!","Config error",MB_OK|MB_ICONINFORMATION);
   return;
  }

 ComboBox_GetLBText(hWC,i,szB);                        // get cd text
 p=szB+1;
 *iA=atoi(p);                                          // get AD,TA,LU
 p=strchr(szB,':')+1;
 *iT=atoi(p);
 p=strchr(p,':')+1;
 *iL=atoi(p);
}

////////////////////////////////////////////////////////////////////////
// interface mode has changed

void OnIMode(HWND hW)
{
 HWND hWC=GetDlgItem(hW,IDC_IMODE);
 int iM  = ComboBox_GetCurSel(hWC);

 GetCDRInfos(hW,&iCD_AD,&iCD_TA,&iCD_LU);              // get sel drive
 CloseGenInterface();                                  // close current interface
 iInterfaceMode=iM;                                    // set new interface mode
 OpenGenInterface();                                   // open new interface
 ComboBox_SetCurSel(hWC,iInterfaceMode);               // sel interface again (maybe it was not supported on open)
 EnumDrives(hW);                                       // enum drives again
}

////////////////////////////////////////////////////////////////////////
// cache mode has changed

void OnCache(HWND hW)
{
 HWND hWC=GetDlgItem(hW,IDC_CACHE);
 if(ComboBox_GetCurSel(hWC)<=0)
      ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_HIDE);
 else ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_SHOW);
}

////////////////////////////////////////////////////////////////////////
// show/hide files depending on subc mode

void ShowSubFileStuff(HWND hW)
{
 HWND hWC=GetDlgItem(hW,IDC_SUBCHAN0);
 int iShow,iSel=ComboBox_GetCurSel(hWC);

 if(iSel==2) iShow=SW_SHOW;
 else        iShow=SW_HIDE; 

 ShowWindow(GetDlgItem(hW,IDC_SFSTATIC),iShow);
 ShowWindow(GetDlgItem(hW,IDC_SUBFILE),iShow);
 ShowWindow(GetDlgItem(hW,IDC_CHOOSESUBF),iShow);

 if(iSel==1)
  {
   ComboBox_SetCurSel(GetDlgItem(hW,IDC_CACHE),0);
   ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_HIDE);
  }
}

////////////////////////////////////////////////////////////////////////
// init dialog

BOOL OnInitCDRDialog(HWND hW) 
{
 HWND hWC;int i=0;

 ReadConfig();                                         // read config

 hWC=GetDlgItem(hW,IDC_IMODE);                         // interface
 ComboBox_AddString(hWC,"NONE");
 ComboBox_AddString(hWC,"W9X/ME - ASPI scsi commands");
 ComboBox_AddString(hWC,"W2K/XP - IOCTL scsi commands");
 ComboBox_AddString(hWC,"W2K/XP - IOCTL raw reading");
 ComboBox_SetCurSel(hWC,iInterfaceMode);

 hWC=GetDlgItem(hW,IDC_RTYPE);                         // read mode
 ComboBox_AddString(hWC,"NONE");
 ComboBox_AddString(hWC,"BE_1 (ATAPI SPEC 1)");
 ComboBox_AddString(hWC,"BE_2 (ATAPI SPEC 2)");
 ComboBox_AddString(hWC,"28_1 (SCSI READ10)");
 ComboBox_AddString(hWC,"28_2 (SCSI READ10 SP)");
 ComboBox_AddString(hWC,"D8_1 (SPECIAL)");
 ComboBox_AddString(hWC,"D4_1 (SPECIAL)");
 ComboBox_AddString(hWC,"D4_2 (SPECIAL)");
 ComboBox_SetCurSel(hWC,iRType);

 EnumDrives(hW);                                       // enum drives

 hWC=GetDlgItem(hW,IDC_CACHE);                         // caching
 ComboBox_AddString(hWC,"None - reads one sector");
 ComboBox_AddString(hWC,"Read ahead - fast, reads more sectors at once");
 ComboBox_AddString(hWC,"Async read - faster, additional asynchronous reads");
 ComboBox_AddString(hWC,"Thread read - fast with IOCTL, always async reads");
 ComboBox_AddString(hWC,"Smooth read - for cd drives with psx cd reading troubles");
 ComboBox_SetCurSel(hWC,iUseCaching);

 if(iUseDataCache)
  CheckDlgButton(hW,IDC_DATACACHE,TRUE);
 if(!iUseCaching)
  ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_HIDE);

 if(iUseSpeedLimit)                                    // speed limit
  CheckDlgButton(hW,IDC_SPEEDLIMIT,TRUE);

 if(iNoWait)                                           // wait for drive
  CheckDlgButton(hW,IDC_NOWAIT,TRUE);

 SetDlgItemInt(hW,IDC_RETRY,iMaxRetry,FALSE);          // retry on error
 if(iMaxRetry)      CheckDlgButton(hW,IDC_TRYAGAIN,TRUE);
 if(iShowReadErr)   CheckDlgButton(hW,IDC_SHOWREADERR,TRUE);

 hWC=GetDlgItem(hW,IDC_SUBCHAN0);                      // subchannel mode
 ComboBox_AddString(hWC,"Don't read subchannels (certain copy-protected games will not work)");
 ComboBox_AddString(hWC,"Read subchannels (slow, few drives support it, best chances with BE mode)");
 ComboBox_AddString(hWC,"Use subchannel SBI/M3S info file (recommended)");
 ComboBox_SetCurSel(hWC,iUseSubReading);

 ShowSubFileStuff(hW);                                 // show/hide subc controls

 hWC=GetDlgItem(hW,IDC_SPEED);                         // speed limit
 ComboBox_AddString(hWC,"2 X");
 ComboBox_AddString(hWC,"4 X");
 ComboBox_AddString(hWC,"8 X");
 ComboBox_AddString(hWC,"16 X");

 i=0;
 if(iSpeedLimit==4)  i=1;
 if(iSpeedLimit==8)  i=2;
 if(iSpeedLimit==16) i=3;

 ComboBox_SetCurSel(hWC,i);

 if(iUsePPF) CheckDlgButton(hW,IDC_USEPPF,TRUE);       // ppf
 SetDlgItemText(hW,IDC_PPFFILE,szPPF);
 SetDlgItemText(hW,IDC_SUBFILE,szSUBF);

 return TRUE;	
}

////////////////////////////////////////////////////////////////////////
// read mode auto detection

void OnCDRAuto(HWND hW) 
{
 int iA,iT,iL,iFoundType=0;char szB[256];
 HWND hWC=GetDlgItem(hW,IDC_RTYPE);

 GetCDRInfos(hW,&iA,&iT,&iL);                          // get drive
 if(iA==-1) return;
 iCD_AD=iA;iCD_TA=iT;iCD_LU=iL;                        // set globals!

 MessageBox(hW,"Please note: you have to insert a PSX CD for the read mode detection","Read mode auto-detection",MB_OK|MB_ICONINFORMATION);

 iUseSubReading=0;                                     // no sub reading on drive check!

 if(IsDlgButtonChecked(hW,IDC_NOWAIT))                 // get wait flag
      iNoWait=1;
 else iNoWait=0;

 if(OpenGenCD(iA,iT,iL,1)>0)                           // open cd
  {
   unsigned char cdb[3000];   
   FRAMEBUF * f=(FRAMEBUF *)cdb;

   f->dwFrame    = 16;                                 // we check on addr 16 (should be available on all psx cds)
   f->dwFrameCnt = 1;  
   f->dwBufLen   = 2352;

   if(CheckSCSIReadMode(&iFoundType,f)!=SS_COMP)       // do the check (will try all available readmodes)
    iFoundType=0;

   if(iFoundType<=0)                                   // no mode
    lstrcpy(szB,"Sorry, no read mode found. You have to try it manually...");
   else
    {                                                  // or show the detected one
     lstrcpy(szB,"Mode found: ");
     ComboBox_GetLBText(hWC,iFoundType,szB+lstrlen(szB));
    }

   CloseGenCD();                                       // close cd again
  }
 else
  {
   lstrcpy(szB,"Can't access cdrom drive!");
  }

 MessageBox(hW,szB,"Read mode auto-detection",MB_OK);
 ComboBox_SetCurSel(hWC,iFoundType);
}

////////////////////////////////////////////////////////////////////////
// SUBCHANNEL STUFF
////////////////////////////////////////////////////////////////////////

void ShowProgress(HWND hW,long lact,long lmin,long lmax)
{
 RECT r;
 HWND hWS=GetDlgItem(hW,IDC_SUBFRAME);
 lmax-=lmin-1;
 lact-=lmin-1;

 GetWindowRect(hWS,&r);
 r.right-=r.left;r.bottom-=r.top;
 ScreenToClient(hW,(LPPOINT)&r);
 r.top+=2;r.left+=2;r.right-=4;r.bottom-=5;
 hWS=GetDlgItem(hW,IDC_SUBFILL);
 if(lmax) r.right=(lact*r.right)/lmax;
 MoveWindow(hWS,r.left,r.top,r.right,r.bottom,TRUE);
 InvalidateRect(hWS,NULL,TRUE);
 UpdateWindow(hWS);
}

//------------------------------------------------------//

void WriteDiffSub(FILE * xfile,int i,unsigned char * lpX,int iM,BOOL b3Min)
{
 unsigned char atime[3],btime[3],ct=1;BOOL b1,b2;

 addr2timeB(i,atime);
 i+=150;
 addr2timeB(i,btime);

 memcpy(&SubCData[12],lpX+2352,16);

 if(iM==0)
  {
   SubCData[15]=itob(SubCData[15]);
   SubCData[16]=itob(SubCData[16]);
   SubCData[17]=itob(SubCData[17]);
   SubCData[19]=itob(SubCData[19]);
   SubCData[20]=itob(SubCData[20]);
   SubCData[21]=itob(SubCData[21]);
  }

 if(b3Min) 
  {
   if(btime[0]!=3) return;
    
   fwrite(&SubCData[12],16,1,xfile);
   return;
  }

 if(atime[0]!=SubCData[15] ||
    atime[1]!=SubCData[16] ||
    atime[2]!=SubCData[17])
  b1=TRUE; else b1=FALSE;

 if(btime[0]!=SubCData[19] ||
    btime[1]!=SubCData[20] ||
    btime[2]!=SubCData[21])
  b2=TRUE; else b2=FALSE;

 if(SubCData[0x0d]!=1) b1=b2=TRUE;
 if(SubCData[0x0e]!=1) b1=b2=TRUE;

 if(b1 || b2)
  {
   fwrite(btime,0x3,1,xfile);
   if(b1 && b2)
    {
     ct=1;
     fwrite(&ct,0x1,1,xfile);
     fwrite(&SubCData[12],10,1,xfile);
    }
   else
   if(b1)
    {
     ct=2;
     fwrite(&ct,0x1,1,xfile);
     fwrite(&SubCData[15],3,1,xfile);
    }
   else
    {
     ct=3;
     fwrite(&ct,0x1,1,xfile);
     fwrite(&SubCData[19],3,1,xfile);
    }
  }
}

//------------------------------------------------------//
//------------------------------------------------------//
//------------------------------------------------------//

BOOL OnCreateSubFileEx(HWND hW,HWND hWX,BOOL b3Min)                              // 
{
 FILE *subfile=NULL;
 FILE *sbifile=NULL;

 unsigned char buf[0x60];char szB[256];BOOL b1,b2;
 unsigned char min,sec,frame,xmin,xsec,xframe;
 unsigned char prevmin,prevsec,prevframe;

 GetDlgItemText(hWX,IDC_SUBFILEEDIT,szB,255);
 if((subfile=fopen(szB,"rb"))==NULL)
  {
   MessageBox(hWX,"Can't open SUB file!","Create SBI/M3S file",MB_OK);
   return 0;
  }

 GetDlgItemText(hWX,IDC_OUTFILEEDIT,szB,255);
 if((sbifile=fopen(szB,"wb"))==NULL)
  {
   MessageBox(hWX,"Can't create out file!","Create SBI/M3S file",MB_OK);
   return 0;
  }

 if(!b3Min)
  {
   strcpy((char *)buf,"SBI");
   fwrite(buf,4,1,sbifile);
  }

 min=0;        // vars for rel address
 sec=1;
 frame=74;

 xmin=0;       // vars for abs address
 xsec=0;
 xframe=255;

 while(!feof(subfile))
  {
   prevmin=min;
   prevsec=sec;
   prevframe=frame;

   xframe=xframe+1;
   if(xframe>74)
    {
     xsec+=1;
     xframe-=75;
     if(xsec>59)
      {
       xmin+=1;
       xsec-=60;
       if(xmin>99) 
        {
         MessageBox(hWX,"SUB file too large!","Create SBI/M3S file",MB_OK);
         return 0;
        }
      }
    }
        
   frame=frame+1;
   if(frame>74)
    {
     sec+=1;
     frame-=75;
     if(sec>59)
      {
       min+=1;
       sec-=60;
       if(min>99) 
        {
         MessageBox(hWX,"SUB file too large!","Create SBI/M3S file",MB_OK);
         return 0;
        }
      }
    }
 
   fread(buf,0x60,1,subfile);

   if(b3Min)
    {
     if(min==3)
      {
       fwrite(&buf[0x0c],16,1,sbifile);
      }
     continue;
    }
   // I only do track 1 
   if(buf[0x0c]==1 && buf[0x0d]==2 && buf[0x0e]==0)
    break;

   if(itod(min)    != buf[0x13]||
      itod(sec)    != buf[0x14]||
      itod(frame)  != buf[0x15])
    b1=TRUE; else b1=FALSE;

   if(itod(xmin)   != buf[0x0f]||
      itod(xsec)   != buf[0x10]||
      itod(xframe) != buf[0x11])
    b2=TRUE; else b2=FALSE;

   if(buf[0x0d]!=1) b1=b2=TRUE;
   if(buf[0x0e]!=1) b1=b2=TRUE;

   if(b1 || b2)
    {
     prevmin=itod(min);
     prevsec=itod(sec);
     prevframe=itod(frame);

     // write real abs address
     fwrite(&prevmin,1,1,sbifile);
     fwrite(&prevsec,1,1,sbifile);
     fwrite(&prevframe,1,1,sbifile);
     // write type 1
     if(b1 && b2)
      {
       prevframe=1;
       fwrite(&prevframe,1,1,sbifile);
       fwrite(&buf[0x0c],10,1,sbifile);
      }
     else if(b2)
      {
       prevframe=2;
       fwrite(&prevframe,1,1,sbifile);
       fwrite(&buf[0x0f],3,1,sbifile);
      }
     else
      {
       prevframe=3;
       fwrite(&prevframe,1,1,sbifile);
       fwrite(&buf[0x13],3,1,sbifile);
      }
    }
  }

 fclose(subfile);
 fclose(sbifile);

 return 1;
}

//------------------------------------------------------//

BOOL OnCreateSubEx(HWND hW,HWND hWX,int iM,BOOL b3Min)                              // 
{
 unsigned char * pB, * pX;
 int iA,iT,iL,iR=0,iSSpeed=1,iFoundType;
 unsigned int i,j;int b;
 HWND hWC=GetDlgItem(hW,IDC_RTYPE);

 GetCDRInfos(hW,&iA,&iT,&iL);
 if(iA==-1) return FALSE;
 iCD_AD=iA;iCD_TA=iT;iCD_LU=iL;

 MessageBox(hWX,"Please insert the PSX CD","Create SBI/M3S file",MB_OK|MB_ICONINFORMATION);

 iFoundType=-1;
 iUseSubReading=0;

 if(IsDlgButtonChecked(hW,IDC_NOWAIT))
      iNoWait=1;
 else iNoWait=0;

 if(OpenGenCD(iA,iT,iL,1)>0)
  {
   FILE * xfile;int iD8Mode;
   unsigned long lStart;
   unsigned long lMAddr=GetLastTrack1Addr();

   if(lMAddr==0)
    {
     CloseGenCD();
     MessageBox(hWX,"Can't read toc!","Create SBI/M3S file",MB_OK);
     return FALSE;
    }

   pB=(unsigned char *)malloc(39168);

   if(SetSCSISpeed(176*2)<=0) iSSpeed=0;

   GetDlgItemText(hWX,IDC_OUTFILEEDIT,(char *)pB,255);
   xfile=fopen((char *)pB,"wb");
   if(xfile==NULL)
    {
     CloseGenCD();
     free(pB);
     MessageBox(hWX,"Can't create out file!","Create SBI/M3S file",MB_OK);
     return FALSE;
    }

   if(!b3Min)
    {
     strcpy((char *)pB,"SBI");
     fwrite(pB,4,1,xfile);
     lStart=1;
    }
   else 
    {
     lStart=13500-150;
     lMAddr=18000-150;
    }

   iM=0;
   iD8Mode=0;
   b=ReadSub_BE_2(0,pB,1);                             // try BE_2
   if(!b)                                              // bad read?
    {
     iD8Mode++;                                        // -> maybe D8
     if(ReadSub_BE_2_1(0,pB,1))                        // -> but try BE_2_1 first
      {
       DecodeSub_BE_2_1(pB+2352);                      // --> decode stuff
       if(*(pB+2352)==0x41) {iM=100;b=TRUE;}           // --> byte ok? fine, try BE_2_1
      }
    }

   if(iM!=100)
    {
     //------------------------------//
     // stupid check, if bcd coded or not
     int iCheck=0;
     b=ReadSub_BE_2(13500-152,pB,16);
     if(!b) iD8Mode++;
     memcpy(&SubCData[12],pB+2352,16);
     if(SubCData[16]==0x57 &&
        SubCData[17]==0x73) iCheck++;
     b=ReadSub_BE_2(13500-151,pB,1);
     if(!b) iD8Mode++;
     memcpy(&SubCData[12],pB+2352,16);
     if(SubCData[16]==0x57 &&
        SubCData[17]==0x74) iCheck++;

     if(iCheck) iM=99;
     //------------------------------//
     b=ReadSub_BE_2(0,pB,1);
     if(!b) iD8Mode++;
     b=ReadSub_BE_2(0,pB,1);
     if(!b) iD8Mode++;

     if(iD8Mode==5)
      {      
       iM=1;          
       ReadSub_D8(0,pB,1);                           
       ReadSub_D8(0,pB,1);
       b=ReadSub_D8(0,pB,1);
      }
     //------------------------------//
    }

   WriteDiffSub(xfile,0,pB,iM,b3Min);

   if(iM==100)
    {
     for(i=lStart;i<lMAddr && b;)
      {                          
       pX=pB;
       b=ReadSub_BE_2_1(i,pB,16);           

       ShowProgress(hWX,i,lStart,lMAddr);

       j=min(i+16,lMAddr);
       for(;i<j;i++)
        {
         DecodeSub_BE_2_1(pX+2352);
         WriteDiffSub(xfile,i,pX,iM,b3Min);
         pX+=2448;
        }
      }
    }
   else
    {      
     for(i=lStart;i<lMAddr && b;)
      {                          
       pX=pB;
       if(iM==0 || iM==99) b=ReadSub_BE_2(i,pB,16);           
       else                b=ReadSub_D8(i,pB,16);            
       ShowProgress(hWX,i,lStart,lMAddr);
       j=min(i+16,lMAddr);
       for(;i<j;i++)
        {
         WriteDiffSub(xfile,i,pX,iM,b3Min);
         pX+=2368;
        }
      }
    }

   fclose(xfile); 

   if(iSSpeed) SetSCSISpeed(0xFFFF);

   CloseGenCD();
   free(pB);
   if(!b)
    {
     MessageBox(hWX,"Error reading subchannels!","Create SBI/M3S file",MB_OK);
     return FALSE;
    }
  }
 else                                 
  {
   MessageBox(hWX,"Can't access cdrom drive!","Create SBI/M3S file",MB_OK);
   return FALSE;
  }
 return TRUE;
}

////////////////////////////////////////////////////////////////////////

void StartSubReading(HWND hW,HWND hWX)
{
 BOOL b=FALSE,b3Min;
 int iM=ComboBox_GetCurSel(GetDlgItem(hWX,IDC_SUBTYPE));

 ShowWindow(GetDlgItem(hWX,IDC_WAITSTATIC),SW_SHOW);
 ShowWindow(GetDlgItem(hWX,IDC_SUBFILL),SW_SHOW);
 ShowWindow(GetDlgItem(hWX,IDC_SUBFRAME),SW_SHOW);

 EnableWindow(GetDlgItem(hWX,IDC_SUBTYPE),FALSE);
 EnableWindow(GetDlgItem(hWX,IDOK),FALSE);
 EnableWindow(GetDlgItem(hWX,IDC_SUBFILEEDIT),FALSE);
 EnableWindow(GetDlgItem(hWX,IDC_CHOOSESUBF),FALSE);
 EnableWindow(GetDlgItem(hWX,IDC_SUBMODE1),FALSE);
 EnableWindow(GetDlgItem(hWX,IDC_SUBMODE2),FALSE);
 EnableWindow(GetDlgItem(hWX,IDC_OUTFILEEDIT),FALSE);
 EnableWindow(GetDlgItem(hWX,IDC_CHOOSEOUTF),FALSE);

 if(IsDlgButtonChecked(hWX,IDC_SUBMODE2)) b3Min=TRUE;
 else                                     b3Min=FALSE;

 if(iM==0) {b=OnCreateSubEx(hW,hWX,iM,b3Min);}  
 else      {b=OnCreateSubFileEx(hW,hWX,b3Min);}  

 if(b) MessageBox(hWX,"Subchannel info file created!","Create SBI/M3S file",MB_OK);
}

////////////////////////////////////////////////////////////////////////

BOOL CALLBACK SubDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_INITDIALOG:
    {
     HWND hWC;int i;
     ShowWindow(GetDlgItem(hW,IDC_WAITSTATIC),SW_HIDE);
     ShowWindow(GetDlgItem(hW,IDC_SUBFILL),SW_HIDE);
     ShowWindow(GetDlgItem(hW,IDC_SUBFRAME),SW_HIDE);
     hWC=GetDlgItem(hW,IDC_SUBTYPE);
     ComboBox_AddString(hWC,"Read subchannels from CD");
     ComboBox_AddString(hWC,"Convert a CloneCD SUB file");
     ComboBox_SetCurSel(hWC,1);
     CheckRadioButton(hW,IDC_SUBMODE1,IDC_SUBMODE2,IDC_SUBMODE1);
     ShowWindow(GetDlgItem(hW,IDC_SUBOUTSTATIC3),SW_HIDE);
     ShowWindow(GetDlgItem(hW,IDC_SUBOUTSTATIC4),SW_HIDE);
     hWC=GetDlgItem(GetParent(hW),IDC_DRIVE);
     i=ComboBox_GetCurSel(hWC);
     if(i!=CB_ERR) 
      {
       char szB[256];
       ComboBox_GetLBText(hWC,i,szB);
       SetDlgItemText(hW,IDC_SUBOUTSTATIC4,szB);
      }
     return TRUE;
    }

   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDC_SUBTYPE:
        if(HIWORD(wParam)==CBN_SELCHANGE) 
         {
          HWND hWC=GetDlgItem(hW,IDC_SUBTYPE);
          int iShow=SW_HIDE;
          if(ComboBox_GetCurSel(hWC)==1) iShow=SW_SHOW;
          ShowWindow(GetDlgItem(hW,IDC_SUBOUTSTATIC),iShow);
          ShowWindow(GetDlgItem(hW,IDC_SUBFILEEDIT),iShow);
          ShowWindow(GetDlgItem(hW,IDC_CHOOSESUBF),iShow);
          if(iShow==SW_SHOW) iShow=SW_HIDE; 
          else               iShow=SW_SHOW;
          ShowWindow(GetDlgItem(hW,IDC_SUBOUTSTATIC3),iShow);
          ShowWindow(GetDlgItem(hW,IDC_SUBOUTSTATIC4),iShow);

          return TRUE;
         }
        break;

       case IDC_CHOOSESUBF:OnChooseFile(hW,2);return TRUE;
       case IDC_CHOOSEOUTF:
        {
         if(IsDlgButtonChecked(hW,IDC_SUBMODE1))
              OnChooseFile(hW,3);
         else OnChooseFile(hW,4);
         return TRUE;
        }

       case IDCANCEL: EndDialog(hW,FALSE);return TRUE;
       case IDOK:     
        {
         PostMessage(GetParent(hW),WM_SUBSTART,0,(LPARAM)hW);
         return TRUE;
        }
      }
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////

void OnCreateSub(HWND hW)                              // 
{
 DialogBox(hInst,MAKEINTRESOURCE(IDD_SUB),
           hW,(DLGPROC)SubDlgProc);
}

////////////////////////////////////////////////////////////////////////

void OnCDROK(HWND hW) 
{
 int iA,iT,iL,iR;
 HWND hWC=GetDlgItem(hW,IDC_RTYPE);

 GetCDRInfos(hW,&iA,&iT,&iL);
 if(iA==-1) return;

 if(IsWindowVisible(hWC))
  {
   iR=ComboBox_GetCurSel(hWC);
   if(iR<=0)
    {
     MessageBox(hW,"Please select a read mode!","Config error",MB_OK|MB_ICONINFORMATION);
     return;
    }
  }
 else iR=1;
 
 hWC=GetDlgItem(hW,IDC_CACHE);
 iUseCaching=ComboBox_GetCurSel(hWC);
 if(iUseCaching<0) iUseCaching=0;
 if(iUseCaching>4) iUseCaching=4;

 iCD_AD=iA;iCD_TA=iT;iCD_LU=iL;iRType=iR;

 if(IsDlgButtonChecked(hW,IDC_SPEEDLIMIT))
      iUseSpeedLimit=1;
 else iUseSpeedLimit=0;

 iUseSubReading=0;
 hWC=GetDlgItem(hW,IDC_SUBCHAN0);
 iUseSubReading=ComboBox_GetCurSel(hWC);
 if(iUseSubReading<0) iUseSubReading=0;
 if(iUseSubReading>2) iUseSubReading=2;
 if(iUseSubReading==1) iUseCaching=0;

 if(IsDlgButtonChecked(hW,IDC_DATACACHE))
      iUseDataCache=1;
 else iUseDataCache=0;
 if(iUseCaching==0) iUseDataCache=0;

 if(IsDlgButtonChecked(hW,IDC_NOWAIT))
      iNoWait=1;
 else iNoWait=0;

 iMaxRetry=GetDlgItemInt(hW,IDC_RETRY,NULL,FALSE);
 if(iMaxRetry<1)  iMaxRetry=1;
 if(iMaxRetry>10) iMaxRetry=10;
 if(!IsDlgButtonChecked(hW,IDC_TRYAGAIN)) iMaxRetry=0;

 if(IsDlgButtonChecked(hW,IDC_SHOWREADERR))
      iShowReadErr=1;
 else iShowReadErr=0;

 hWC=GetDlgItem(hW,IDC_SPEED);
 iR=ComboBox_GetCurSel(hWC);

 iSpeedLimit=2;
 if(iR==1) iSpeedLimit=4;
 if(iR==2) iSpeedLimit=8;
 if(iR==3) iSpeedLimit=16;

 if(IsDlgButtonChecked(hW,IDC_USEPPF))
      iUsePPF=1;
 else iUsePPF=0;

 GetDlgItemText(hW,IDC_PPFFILE,szPPF,259);
 GetDlgItemText(hW,IDC_SUBFILE,szSUBF,259);

 WriteConfig();                                        // write registry

 EndDialog(hW,TRUE);
}

////////////////////////////////////////////////////////////////////////

void OnCDRCancel(HWND hW) 
{
 EndDialog(hW,FALSE);
}

////////////////////////////////////////////////////////////////////////

BOOL CALLBACK CDRDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_SUBSTART:
    {
     HWND hWX=(HWND)lParam;
     StartSubReading(hW,hWX);
     PostMessage(hWX,WM_COMMAND,IDCANCEL,0);
     return TRUE;
    }

   case WM_INITDIALOG:
     return OnInitCDRDialog(hW);

   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDC_SUBCHAN0:   if(HIWORD(wParam)==CBN_SELCHANGE) 
                             {ShowSubFileStuff(hW);return TRUE;}
       case IDC_IMODE:      if(HIWORD(wParam)==CBN_SELCHANGE) 
                             {OnIMode(hW);return TRUE;}
                            break;
       case IDC_CACHE:      if(HIWORD(wParam)==CBN_SELCHANGE) 
                             {OnCache(hW);return TRUE;}
                            break;
       case IDCANCEL:       OnCDRCancel(hW); return TRUE;
       case IDOK:           OnCDROK(hW);     return TRUE;
       case IDC_AUTO:       OnCDRAuto(hW);   return TRUE;
       case IDC_CHOOSEFILE: OnChooseFile(hW,0);return TRUE;
       case IDC_CHOOSESUBF: OnChooseFile(hW,1);return TRUE;
       case IDC_CREATESUB:  OnCreateSub(hW);   return TRUE;
      }
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDCANCEL: EndDialog(hW,FALSE);return TRUE;
       case IDOK:     EndDialog(hW,FALSE);return TRUE;
      }
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////

