#ifndef _DICOMRead_H
#define _DICOMRead_H

#include "dicom.h"
#include "lst.h"
#include "dicom_objects.h"
#include "condition.h"


#define NUMBEROFTAGS 22
#define SHORTSIZE 16
#define INTSIZE 16
#define LONGSIZE 32

#define DCM_NOERROR 0
#define DCM_NODICOMFILES 1
#define DCM_MULTIPLESTUDIES 2

typedef unsigned short int BOOL;
typedef unsigned short int bool;

#define true 1
#define false 0

#ifdef _DICOMRead_SRC
  char *SDCMStatusFile;
  char  *SDCMListFile;
#else
  extern char *SDCMStatusFile;
  extern char  *SDCMListFile;
#endif

typedef enum {
  // general infos
  DCM_StudyDate, DCM_PatientName, DCM_Manufacturer, DCM_StudyTime, DCM_SeriesTime, DCM_AcquisitionTime,
  
  // image dimensions
  DCM_SliceThickness, DCM_xsize, DCM_ysize, DCM_ImageNumber, DCM_Rows, DCM_Columns, DCM_BitsAllocated, 
  DCM_NumberOfFrames, DCM_FieldOfView,

  // image position and orientation
  DCM_ImagePosition,  DCM_ImageOrientation,

  // acquisition parameters
  DCM_EchoTime, DCM_RepetitionTime, DCM_InversionTime, DCM_FlipAngle, DCM_EchoNumber
} DCM_TagList;

typedef struct
{
  // DICOM file name
  char *FileName;
  
  // general infos
  char *StudyDate, 
    *PatientName, 
    *Manufacturer, 
    *StudyTime, 
    *SeriesTime, 
    *AcquisitionTime;
  
  // image dimensions
  double SliceThickness,
    xsize,
    ysize,
    FieldOfView;
  unsigned short ImageNumber,
    Rows, 
    Columns, 
    BitsAllocated, 
    NumberOfFrames;

  // image position and orientation
  double ImagePosition[3], 
    ImageOrientation[6],
    FirstImagePosition[3],
    LastImagePosition[3];

  // acquisition parameters
  double EchoTime, 
    RepetitionTime,
    InversionTime,
    FlipAngle;
  short EchoNumber;

  // pixels
  void *PixelData;
  unsigned char min8, 
    max8;
  unsigned short int min16, 
    max16;

}  DICOMInfo ;

/*--- Relevant Info for a single Siemens DICOM File ------*/
typedef struct {
  char *FileName;
  char *PatientName;
  char *StudyDate;
  char *StudyTime;
  char *SeriesTime;
  char *AcquisitionTime;
  char *PulseSequence;
  char *ProtocolName;
  char *PhEncDir;

  int   EchoNo;
  float FlipAngle;
  float EchoTime;
  float RepetitionTime;
  float InversionTime;

  float PhEncFOV;
  float ReadoutFOV;
  
  int  SeriesNo;
  int  ImageNo;
  int  NImageRows;
  int  NImageCols;
  float ImgPos[3];

  int  lRepetitions;
  int  SliceArraylSize;

  /* Volume/Run Related parameters */
  float Vc[3];     /* RAS col direction cosines*/
  float Vr[3];     /* RAS row direction cosines*/
  float Vs[3];     /* RAS slice direction cosines*/

  int   RunNo;       /* Run Number that this is associated with */
  int   IsMosaic;    /* Image is a mosaic of slices */
  int   VolDim[3];   /* number of cols rows slices */
  float VolRes[3];   /* Resolution of col, row, slice in mm */
  float VolCenter[3]; /* Exact RAS center of the volume */
  int   NFrames;     /* Equals lRepetitions + 1 */
  
  int   ErrorFlag;   /* Set for error, eg, aborted run */

} SDCMFILEINFO;



void PrintDICOMInfo(DICOMInfo *dcminfo);
CONDITION GetString(DCM_OBJECT** object, DCM_TAG tag, char **st);
CONDITION GetUSFromString(DCM_OBJECT** object, DCM_TAG tag, unsigned short *us);
CONDITION GetShortFromString(DCM_OBJECT** object, DCM_TAG tag, short *sh);
CONDITION GetUSFromUS(DCM_OBJECT** object, DCM_TAG tag, unsigned short *us);
CONDITION GetShortFromShort(DCM_OBJECT** object, DCM_TAG tag, short *ss);
CONDITION GetPixelData_Save(DCM_OBJECT** object, DCM_TAG tag, unsigned short **ad);
CONDITION GetPixelData(DCM_OBJECT** object, DCM_TAG tag, void **ad);
CONDITION GetDoubleFromString(DCM_OBJECT** object, DCM_TAG tag, double *d);
CONDITION GetMultiDoubleFromString(DCM_OBJECT** object, DCM_TAG tag, double *d[], int multiplicity);
CONDITION GetMultiShortFromString(DCM_OBJECT** object, DCM_TAG tag, short *us[], int multiplicity);
CONDITION GetDICOMInfo(char *fname, DICOMInfo *dcminfo, BOOL ReadImage, int ImageNumber);
void *ReadDICOMImage(int nfiles, DICOMInfo **aDicomInfo);
void SortFiles(char *fNames[], int nFiles, DICOMInfo ***ptrDicomArray, int *nStudies);
int IsDICOM(char *fname);
int ScanDir(char *PathName, char ***FileNames, int *NumberOfFiles);
int CleanFileNames(char **FileNames, int NumberOfDICOMFiles, char ***CleanedFileNames);
int DICOMRead(char *FileName, MRI **mri, int ReadImage);

DCM_ELEMENT *GetElementFromFile(char *dicomfile, long grpid, long elid);
int AllocElementData(DCM_ELEMENT *e);
int FreeElementData(DCM_ELEMENT *e);
DCM_ELEMENT *GetElementFromFile(char *dicomfile, long grpid, long elid);
DCM_OBJECT *GetObjectFromFile(char *fname, unsigned long options);
int IsSiemensDICOM(char *dcmfile);
char *SiemensAsciiTag(char *dcmfile, char *TagString);
int dcmGetNCols(char *dcmfile);
int dcmGetNRows(char *dcmfile);
int dcmGetVolRes(char *dcmfile, float *ColRes, float *RowRes, float *SliceRes);
float sdcmMosaicSliceRes(char *dcmfile);
int dcmImageDirCos(char *dcmfile, 
       float *Vcx, float *Vcy, float *Vcz,
       float *Vrx, float *Vry, float *Vrz);
int sdcmSliceDirCos(char *dcmfile, float *Vsx, float *Vsy, float *Vsz);
int dcmImagePosition(char *dcmfile, float *x, float *y, float *z);

int sdcmIsMosaic(char *dcmfile, int *pNcols, int *pNrows, int *pNslices, int *pNframes);

int DumpSDCMFileInfo(FILE *fp, SDCMFILEINFO *sdcmfi);
int FreeSDCMFileInfo(SDCMFILEINFO **ppsdcmfi);
SDCMFILEINFO *GetSDCMFileInfo(char *dcmfile);
SDCMFILEINFO **ScanSiemensDCMDir(char *PathName, int *NSDCMFiles);
int CompareSDCMFileInfo(const void *a, const void *b);
int SortSDCMFileInfo(SDCMFILEINFO **sdcmfi_list, int nlist);

int sdfiAssignRunNo(SDCMFILEINFO **sdcmfi_list, int nfiles);
int sdfiAssignRunNo2(SDCMFILEINFO **sdfi_list, int nlist);
int sdfiRunNo(char *dcmfile, SDCMFILEINFO **sdfi_list, int nlist);
int sdfiNFilesInRun(char *dcmfile, SDCMFILEINFO **sdfi_list, int nlist);
int sdfiCountFilesInRun(int RunNo, SDCMFILEINFO **sdfi_list, int nlist);
int *sdfiRunFileList(char *dcmfile, SDCMFILEINFO **sdfi_list, 
         int nlist, int *NRunList);
MRI * sdcmLoadVolume(char *dcmfile, int LoadVolume);
int sdfiVolCenter(SDCMFILEINFO *sdfi);
int sdfiFixImagePosition(SDCMFILEINFO *sdfi);
int sdfiSameSlicePos(SDCMFILEINFO *sdfi1, SDCMFILEINFO *sdfi2);
int sdfiCountRuns(SDCMFILEINFO **sdfi_list, int nlist);
char *sdfiFirstFileInRun(int RunNo, SDCMFILEINFO **sdfi_list, int nlist);
int *sdfiRunNoList(SDCMFILEINFO **sdfi_list, int nlist, int *NRuns);
char **ScanSiemensSeries(char *dcmfile, int *nList);
char **ReadSiemensSeries(char *ListFile, int *nList, char *dcmfile);
SDCMFILEINFO **LoadSiemensSeriesInfo(char **SeriesList, int nList);

#ifdef Solaris
int scandir(const char *dir, struct dirent ***namelist,
            int (*select)(const struct dirent *),
            int (*compar)(const void *, const void *));
int alphasort(const void *a, const void *b);
#endif

#endif
