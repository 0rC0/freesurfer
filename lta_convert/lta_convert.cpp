/**
 * @file  lta_convert.cpp
 * @brief A programm to convert linear transform file formats
 *
 */

/*
 * Original Author: Martin Reuter
 * CVS Revision Info:
 *    $Author: mreuter $
 *    $Date: 2013/04/25 17:40:03 $
 *    $Revision: 1.1 $
 *
 * Copyright © 2011 The General Hospital Corporation (Boston, MA) "MGH"
 *
 * Terms and conditions for use, reproduction, distribution and contribution
 * are found in the 'FreeSurfer Software License Agreement' contained
 * in the file 'LICENSE' found in the FreeSurfer distribution, and here:
 *
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferSoftwareLicense
 *
 * Reporting: freesurfer@nmr.mgh.harvard.edu
 *
 */

#include <string>
#include <iostream>

// all other software are all in "C"
#ifdef __cplusplus
extern "C"
{
#endif

#include "error.h"
#include "macros.h"
#include "mri.h"
#include "transform.h"
#include "resample.h"
#include "registerio.h"
#include "version.h"

#ifdef __cplusplus
}
#endif

using namespace std;

namespace intypes {
enum InputType { UNKNOWN, LTA, REG, FSL, MNI };
}

struct Parameters
{
  string transin;
  string ltaout;
  string fslout;
  string mniout;
  string regout;
  string src;
  string trg;
  bool   invert;
  int    ltaouttype;
  bool   trgconform;
  intypes::InputType intype;
};

static struct Parameters P =
{ "", "", "", "", "" ,"" ,"" , false , LINEAR_RAS_TO_RAS, false, intypes::UNKNOWN};

static void printUsage(void);
static bool parseCommandLine(int argc, char *argv[], Parameters & P);

static char vcid[] =
    "$Id: lta_convert.cpp,v 1.1 2013/04/25 17:40:03 mreuter Exp $";
char *Progname = NULL;

LTA * shallowCopyLTA(const LTA * lta)
{
  LTA * ltatmp = LTAalloc(1,NULL);
  ltatmp->xforms[0].m_L=MatrixCopy(lta->xforms[0].m_L,NULL);
  copyVolGeom(&lta->xforms[0].src,&ltatmp->xforms[0].src);
  copyVolGeom(&lta->xforms[0].dst,&ltatmp->xforms[0].dst);
  ltatmp->type = lta->type;
  ltatmp->fscale = lta->fscale;
  strcpy(ltatmp->subject, lta->subject); 
  return ltatmp;
}

LTA * readLTA(const string& xfname, const string& sname, const string& tname)
{
  LTA* lta = LTAread(P.transin.c_str());
  if (lta == NULL)
  {
    cerr << "ERROR readLTA: cannot read " << xfname << endl;
    exit(1);
  }
  
  if (lta->type != LINEAR_RAS_TO_RAS)
    LTAchangeType(lta, LINEAR_RAS_TO_RAS);  
  if (lta->type != LINEAR_RAS_TO_RAS)
  {
    cerr << "ERROR readLTA: cannot change type to RAS_TO_RAS." << endl;
    exit(1);  
  }
  
  // if src and trg mri are passed, change geometry
  if (sname != "")
  {
    MRI * src = MRIreadHeader(sname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
    if (src == NULL)
    {
      cerr << "ERROR readLTA: cannot read src MRI" << sname << endl;
      exit(1);
    }
    getVolGeom(src, &lta->xforms[0].src);
    MRIfree(&src);
  }
  if (tname != "")
  { 
    MRI * trg = MRIreadHeader(tname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
    if (trg == NULL)
    {
      cerr << "ERROR readFSL: cannot read trg MRI" << tname << endl;
      exit(1);
    }
    getVolGeom(trg, &lta->xforms[0].dst);
    MRIfree(&trg);
  }
  return lta;
}

LTA * readFSL(const string& xfname, const string& sname, const string& tname)
// this is based on a conversion to tkreg matrix  using MRIfsl2TkReg 
// and then an lta change type from REGISTER_DAT
{
  FILE *fp = fopen(xfname.c_str(),"r");
  if (fp == NULL)
  {
    cerr << "ERROR readFSL: cannot open "<< xfname << endl;
    exit(1);
  }

  MATRIX *FSLRegMat= MatrixAlloc(4,4,MATRIX_REAL);
  int i,j,n;
  for (i=0;i<4;i++)
  {
    for (j=0;j<4;j++)
    {
      n = fscanf(fp,"%f",&(FSLRegMat->rptr[i+1][j+1]));
      if (n != 1)
      {
        cerr << "ERROR readFSL: reading "<<xfname<<", row "<<i<<", col " << j << endl;
        exit(1);
      }
    }
  }

  cout << "------ FSL registration matrix --------" << endl;
  MatrixPrint(stdout,FSLRegMat);
  cout << "---------------------------------------" << endl;

  // read src and target mri header
  MRI * src = MRIreadHeader(sname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
  if (src == NULL)
  {
    cerr << "ERROR readFSL: cannot read src MRI" << sname << endl;
    exit(1);
  }
  MRI * trg = MRIreadHeader(tname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
  if (trg == NULL)
  {
    cerr << "ERROR readFSL: cannot read trg MRI" << tname << endl;
    exit(1);
  }

  // convert to reg mat
  MATRIX * RegMat = MRIfsl2TkReg(trg, src, FSLRegMat);

  cout << "------- TK registration matrix --------" << endl;
  MatrixPrint(stdout,RegMat);
  cout << "---------------------------------------" << endl;
  
  LTA *lta = LTAalloc(1, NULL) ;
  lta->xforms[0].m_L = MatrixCopy(RegMat, NULL) ;
  getVolGeom(src, &lta->xforms[0].src);
  getVolGeom(trg, &lta->xforms[0].dst);
  lta->type = REGISTER_DAT;
  lta = LTAchangeType(lta, LINEAR_RAS_TO_RAS);

  MatrixFree(&FSLRegMat);
  MatrixFree(&RegMat);
  MRIfree(&src);
  MRIfree(&trg);

  return lta;

}


LTA * readMNI(const string& xfname, const string& sname, const string& tname)
// based on regio_read_mincxfm for reading the matrix
// then the matrix should be RAS2RAS
{

  MATRIX * MNIRegMat = NULL;
  int err = regio_read_mincxfm(xfname.c_str(), &MNIRegMat, NULL);
  if (err || MNIRegMat == NULL)
  {
    cerr << "ERROR readMNI: cannot read "<< xfname << endl;
    exit(1);
  }

  cout << "------ MNI registration matrix --------" << endl;
  MatrixPrint(stdout,MNIRegMat);
  cout << "---------------------------------------" << endl;

  // read src and target mri header
  MRI * src = MRIreadHeader(sname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
  if (src == NULL)
  {
    cerr << "ERROR readFSL: cannot read src MRI" << sname << endl;
    exit(1);
  }
  MRI * trg = MRIreadHeader(tname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
  if (trg == NULL)
  {
    cerr << "ERROR readFSL: cannot read trg MRI" << tname << endl;
    exit(1);
  }
  
  // NMI XFM matrix should be identical with RAS2RAS?
  LTA *lta = LTAalloc(1, NULL) ;
  lta->xforms[0].m_L = MatrixCopy(MNIRegMat, NULL) ;
  getVolGeom(src, &lta->xforms[0].src);
  getVolGeom(trg, &lta->xforms[0].dst);
  lta->type = LINEAR_RAS_TO_RAS;

  MatrixFree(&MNIRegMat);
  MRIfree(&src);
  MRIfree(&trg);

  return lta;

}

LTA * readREG(const string& xfname, const string& sname, const string& tname)
//based on regio_read_register for reading and then lta change type from REGISTER_DAT
{

  MATRIX * RegMat = NULL;
  char *sidstr;
  float ipr, bpr, fscale;
  int float2int;
  int err = regio_read_register(xfname.c_str(), &sidstr, &ipr, &bpr, 
                                &fscale, &RegMat, &float2int);
  if (err || RegMat == NULL)
  {
    cerr << "ERROR readREG: cannot read "<< xfname << endl;
    exit(1);
  }

  cout << "------ REG registration matrix --------" << endl;
  MatrixPrint(stdout,RegMat);
  cout << "---------------------------------------" << endl;

  // read src and target mri header
  MRI * src = MRIreadHeader(sname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
  if (src == NULL)
  {
    cerr << "ERROR readFSL: cannot read src MRI" << sname << endl;
    exit(1);
  }
  MRI * trg = MRIreadHeader(tname.c_str(),MRI_VOLUME_TYPE_UNKNOWN);
  if (trg == NULL)
  {
    cerr << "ERROR readFSL: cannot read trg MRI" << tname << endl;
    exit(1);
  }
  
  // NMI XFM matrix should be identical with RAS2RAS?
  LTA *lta = LTAalloc(1, NULL) ;
  lta->xforms[0].m_L = MatrixCopy(RegMat, NULL) ;
  getVolGeom(src, &lta->xforms[0].src);
  getVolGeom(trg, &lta->xforms[0].dst);
  lta->type = REGISTER_DAT;
  lta->fscale = fscale;
  strcpy(lta->subject,sidstr);
  lta = LTAchangeType(lta, LINEAR_RAS_TO_RAS);

  MatrixFree(&RegMat);
  MRIfree(&src);
  MRIfree(&trg);
  free(sidstr);

  return lta;
}


void writeFSL(const string& fname, const LTA * lta)
{
//   // shallow copy
//   LTA * ltatmp = shallowCopyLTA(lta);
//   
//   if (ltatmp->type != FSLREG_TYPE)
//     LTAchangeType(ltatmp, FSLREG_TYPE);
// 
//   FILE *fp = fopen(fname.c_str(),"w");
//   if (fp == NULL) {
//     cerr << "ERROR: cannot open FSL output "<<fname<<" for writing\n";
//     return;
//   }
//   int i,j;
//   for (i=0;i<4;i++) {
//     for (j=0;j<4;j++)
//       fprintf(fp,"%13.8f ",ltatmp->xforms[0].m_L->rptr[i+1][j+1]);
//     fprintf(fp,"\n");
//   }
//   fclose(fp);
//   LTAfree(&ltatmp);
//   return;

  MRI * src = MRIallocHeader(1,1,1,MRI_FLOAT,1);
  useVolGeomToMRI(&lta->xforms[0].dst,src);
  MRI * trg = MRIallocHeader(1,1,1,MRI_FLOAT,1);
  useVolGeomToMRI(&lta->xforms[0].src,trg);

  if (lta->type != LINEAR_RAS_TO_RAS)
  {
    cerr << "ERROR: lta should be RAS_TO_RAS by now!!!"<< endl;
    exit(1);  
  }

  MATRIX *Mreg = MRItkRegMtx(trg, src, lta->xforms[0].m_L);
  MATRIX *Mfsl = MRItkreg2FSL(trg, src, Mreg);

  FILE *fp = fopen(fname.c_str(),"w");
  if (fp == NULL) {
    printf("ERROR: cannot open %s for writing\n",fname.c_str());
    return;
  }
  int i,j;
  for (i=0;i<4;i++) {
    for (j=0;j<4;j++)
      fprintf(fp,"%13.8f ",Mfsl->rptr[i+1][j+1]);
    fprintf(fp,"\n");
  }
  fclose(fp);

  MatrixFree(&Mreg);
  MatrixFree(&Mfsl);
  MRIfree(&src);
  MRIfree(&trg);

  return;
}

void writeMNI(const string& fname, const LTA * lta)
// this is the xfm format
{
//   // shallow copy
//   LTA * ltatmp = shallowCopyLTA(lta);
//   
//   if (ltatmp->type != MNI_TRANSFORM_TYPE)
//     LTAchangeType(ltatmp, MNI_TRANSFORM_TYPE);
// 
//   FILE *fp = fopen(fname.c_str(),"w");
//   if (fp == NULL) {
//     cerr << "ERROR: cannot open MNI output "<<fname<<" for writing\n";
//     return;
//   }
//   fprintf(fp,"MNI Transform File\n");
//   fprintf(fp,"%% lta_convert\n");
//   fprintf(fp,"\n");
//   fprintf(fp,"Transform_Type = Linear;\n");
//   fprintf(fp,"Linear_Transform =\n");
//   int i,j;
//   for (i=0;i<3;i++) {
//     for (j=0;j<4;j++)
//       fprintf(fp,"%13.8f ",ltatmp->xforms[0].m_L->rptr[i+1][j+1]);
//     if (i != 2) fprintf(fp,"\n");
//     else       fprintf(fp,";\n");
//   }
//   
//   
//   fclose(fp);
//   LTAfree(&ltatmp);
//   return;
  
  MRI * src = MRIallocHeader(1,1,1,MRI_FLOAT,1);
  useVolGeomToMRI(&lta->xforms[0].dst,src);
  MRI * trg = MRIallocHeader(1,1,1,MRI_FLOAT,1);
  useVolGeomToMRI(&lta->xforms[0].src,trg);

  if (lta->type != LINEAR_RAS_TO_RAS)
  {
    cerr << "ERROR: lta should be RAS_TO_RAS by now!!!"<< endl;
    exit(1);  
  }
  MATRIX *Mreg = MRItkRegMtx(trg, src, lta->xforms[0].m_L);
  MATRIX *Mmni = MRItkReg2Native(trg, src, Mreg);
  
  // should be identical to ras to ras:
  cout << "------ MNI registration matrix --------" << endl;
  MatrixPrint(stdout,Mmni);
  cout << "---------------------------------------" << endl;
  cout << "------ LTA registration matrix --------" << endl;
  MatrixPrint(stdout,lta->xforms[0].m_L);
  cout << "---------------------------------------" << endl;
  
  FILE *fp = fopen(fname.c_str(),"w");
  if (fp == NULL) {
    printf("ERROR: cannot open %s for writing\n",fname.c_str());
    return;
  }
  fprintf(fp,"MNI Transform File\n");
  fprintf(fp,"%% lta_convert\n");
  fprintf(fp,"\n");
  fprintf(fp,"Transform_Type = Linear;\n");
  fprintf(fp,"Linear_Transform =\n");
  int i,j;
  for (i=0;i<3;i++) {
    for (j=0;j<4;j++)
      fprintf(fp,"%13.8f ",Mmni->rptr[i+1][j+1]);
    if (i != 2) fprintf(fp,"\n");
    else       fprintf(fp,";\n");
  }
  fclose(fp);

  MatrixFree(&Mreg);
  MatrixFree(&Mmni);
  MRIfree(&src);
  MRIfree(&trg);

  return;
  
}

void writeREG(const string& fname, const LTA * lta)
//RegMat = MRItkRegMtx(targ_vol,mov_vol,XFM);
{
//   // shallow copy
//   LTA * ltatmp = shallowCopyLTA(lta);
//     
//   if (ltatmp->type != REGISTER_DAT)
//     LTAchangeType(ltatmp, REGISTER_DAT);
// 
//   int err ;
//   err =  regio_write_register((char*)fname.c_str(), (char *)ltatmp->subject, ltatmp->xforms[0].src.xsize,
//                               ltatmp->xforms[0].src.zsize, ltatmp->fscale, ltatmp->xforms[0].m_L,
//                               FLT2INT_ROUND);
//   
//   if (err != 0)
//   {
//     cerr << "ERROR: exporting REG dat output "<<fname<< endl;
//     return;
//   }
//   LTAfree(&ltatmp);
//   return;

  MRI * src = MRIallocHeader(1,1,1,MRI_FLOAT,1);
  useVolGeomToMRI(&lta->xforms[0].dst,src);
  MRI * trg = MRIallocHeader(1,1,1,MRI_FLOAT,1);
  useVolGeomToMRI(&lta->xforms[0].src,trg);

  if (lta->type != LINEAR_RAS_TO_RAS)
  {
    cerr << "ERROR: lta should be RAS_TO_RAS by now!!!"<< endl;
    exit(1);  
  }
  MATRIX *Mreg = MRItkRegMtx(trg, src, lta->xforms[0].m_L);

  LTA* ltatmp = shallowCopyLTA(lta);
  ltatmp->xforms[0].m_L=MatrixCopy(Mreg,ltatmp->xforms[0].m_L);
  ltatmp->type = REGISTER_DAT;
  
  if(LTAwrite(ltatmp, fname.c_str()) != NO_ERROR)
  {
    cerr << "ERROR writeREG: cannot create file " << fname << endl;
    exit(1);
  }

  MatrixFree(&Mreg);
  MRIfree(&src);
  MRIfree(&trg);
  LTAfree(&ltatmp);

  return;
}


int main(int argc, char *argv[])
{
  cout << vcid << endl << endl;

  // Default initialization
  int nargs = handle_version_option(argc, argv, vcid, "$Name:  $");
  if (nargs && argc - nargs == 1)
  {
    exit(0);
  }
  argc -= nargs;
  Progname = argv[0];
  argc--;
  argv++;
  ErrorInit(NULL, NULL, NULL);
    
  // Parse command line
  if (!parseCommandLine(argc, argv, P))
  {
    //printUsage();
    exit(1);
  }
  
  // Read input transform and convert to RAS2RAS:
  LTA * lta = NULL;  
  if (P.intype==intypes::LTA)
    lta = readLTA(P.transin.c_str(),P.src,P.trg);
  else if (P.intype==intypes::FSL)
    lta = readFSL(P.transin.c_str(),P.src,P.trg);
  else if (P.intype==intypes::MNI)
    lta = readMNI(P.transin.c_str(),P.src,P.trg);
  else if (P.intype==intypes::REG)
    lta = readREG(P.transin.c_str(),P.src,P.trg);
  if (!lta)
  {
    ErrorExit(ERROR_BADFILE, "%s: can't read input file %s",Progname, P.transin.c_str());
  }
  if (lta->type != LINEAR_RAS_TO_RAS)
  {
    cerr << "ERROR: lta should be RAS_TO_RAS by now!!!"<< endl;
    exit(1);  
  }
  cout << " LTA read, type : " << lta->type << endl;
  MatrixPrint(stdout,lta->xforms[0].m_L);
  
  // conform trg
  if (P.trgconform)
    initVolGeom(&lta->xforms[0].dst);
  
  // invert if desired
  if (P.invert)
  {
    VOL_GEOM vgtmp;
    MATRIX *m_tmp = lta->xforms[0].m_L ;
    lta->xforms[0].m_L = MatrixInverse(lta->xforms[0].m_L, NULL) ;
    MatrixFree(&m_tmp) ;
    LT *lt = &lta->xforms[0];
    if (lt->dst.valid == 0 || lt->src.valid == 0)
    {
      cerr << "WARNING:********************************************************\n";
      cerr << "WARNING: dst or src volume is invalid.  Inverse likely wrong.\n";
      cerr << "WARNING:********************************************************\n";
    }
    copyVolGeom(&lt->dst, &vgtmp);
    copyVolGeom(&lt->src, &lt->dst);
    copyVolGeom(&vgtmp, &lt->src);  
  }
    
  // write final
  if (P.ltaout!="")
  {
    if (lta->type != P.ltaouttype) // can only be ras2ras (default) or vox2vox here
    {
      LTAchangeType(lta, P.ltaouttype);
    }
    cout << "Writing  LTA to file "<<P.ltaout.c_str()<<"...\n";
    FILE* fo = fopen(P.ltaout.c_str(),"w");
    if (fo==NULL)
      ErrorExit(ERROR_BADFILE,
                "%s: can't create file %s",Progname, P.ltaout.c_str());
    LTAprint(fo, lta);
    fclose(fo);
  }
  
  if (P.fslout!="")
  {
    writeFSL(P.fslout,lta);
  }
  if (P.mniout!="")
  {
    writeMNI(P.mniout,lta);
  }
  if (P.regout!="")
  {
    writeREG(P.regout,lta);
  }
  
  LTAfree(&lta);
  printf("%s successful.\n", Progname);
  return (0);
}

#include "lta_convert.help.xml.h"
static void printUsage(void)
{
  outputHelpXml(lta_convert_help_xml, lta_convert_help_xml_len);
}

/*!
 \fn int parseNextCommand(int argc, char **argv)
 \brief Parses the command-line for next command
 \param   argc  number of command line arguments
 \param   argv  pointer to a character pointer
 \param      P  reference to parameters
 \returns       number of used arguments for this command
 */
static int parseNextCommand(int argc, char *argv[], Parameters & P)
{
  int nargs = 0;
  char *option;

  option = argv[0] + 1;                     // remove '-'
  if (option[0] == '-')
  {
    option = option + 1;  // remove second '-'
  }
  StrUpper(option);

  //cout << " option: " << option << endl;

//  if (!strcmp(option, "IN") )
//  {
//    P.transin = string(argv[1]);
//    nargs = 1;
//    cout << "--in: " << P.transin << " input transform." << endl;
//  }
  if (!strcmp(option, "INLTA") )
  {
    P.transin = string(argv[1]);
    P.intype = intypes::LTA;
    nargs = 1;
    cout << "--inlta: " << P.transin << " input LTA transform." << endl;
  }
  else if (!strcmp(option, "INFSL") )
  {
    P.transin = string(argv[1]);
    P.intype = intypes::FSL;
    nargs = 1;
    cout << "--infsl: " << P.transin << " input FSL transform." << endl;
  }
  else if (!strcmp(option, "INMNI") || !strcmp(option, "INXFM"))
  {
    P.transin = string(argv[1]);
    P.intype = intypes::MNI;
    nargs = 1;
    cout << "--inmni: " << P.transin << " input MNI/XFM transform." << endl;
  }
  else if (!strcmp(option, "INREG"))
  {
    P.transin = string(argv[1]);
    P.intype = intypes::REG;
    nargs = 1;
    cout << "--inreg: " << P.transin << " input TK REG transform." << endl;
  }
  else if (!strcmp(option, "OUTLTA") )
  {
    P.ltaout = string(argv[1]);
    nargs = 1;
    cout << "--outlta: " << P.ltaout << " output LTA." << endl;
  }
  else if (!strcmp(option, "OUTFSL") )
  {
    P.fslout = string(argv[1]);
    nargs = 1;
    cout << "--outfsl: " << P.fslout << " output FSL matrix." << endl;
  }
  else if (!strcmp(option, "OUTMNI") )
  {
    P.mniout = string(argv[1]);
    nargs = 1;
    cout << "--outmni: " << P.mniout << " output MNI/XFM matrix." << endl;
  }
  else if (!strcmp(option, "OUTREG") )
  {
    P.regout = string(argv[1]);
    nargs = 1;
    cout << "--outreg: " << P.regout << " output reg.dat matrix." << endl;
  }
  else if (!strcmp(option, "SRC") )
  {
    P.src = string(argv[1]);
    nargs = 1;
    cout << "--src: " << P.src << " src image (geometry)." << endl;
  }
  else if (!strcmp(option, "TRG") )
  {
    P.trg = string(argv[1]);
    nargs = 1;
    cout << "--trg: " << P.trg << " trg image (geometry)." << endl;
  }
  else if (!strcmp(option, "TRGCONFORM") )
  {
    P.trgconform = true;
    cout << "--trgconform: will conform target geometry." << endl;
  }
  else if (!strcmp(option, "LTAVOX2VOX") )
  {
    P.ltaouttype = LINEAR_VOX_TO_VOX;
    cout << "--ltavox2vox: output LTA as VOX_TO_VOX transform." << endl;
  }
  else if (!strcmp(option, "INVERT") )
  {
    P.invert = true;
    cout << "--invert: will invert transform." << endl;
  }
//  else if (!strcmp(option, "INTYPE") )
//  {
//    char* it = argv[1];
//    StrUpper(it);
//    nargs = 1;
//    string sit = string(it);
//    if (sit == "LTA") P.intype = intypes::LTA;
//    else if (sit == "MNI") P.intype = intypes::MNI;
//    else if (sit == "XFM") P.intype = intypes::MNI;
//    else if (sit == "FSL") P.intype = intypes::FSL;
//    else if (sit == "REG") P.intype = intypes::REG;
//    else
//    {
//      cout << "WARNING: intype " << sit << "unknown, will try to detect input..." << endl;
//      P.intype = intypes::UNKNOWN;
//    }
//    cout << "--intype: " << sit << endl;
//  }
  else
  {
    cerr << endl << endl << "ERROR: Option: " << argv[0] << " unknown !! "
        << endl << endl;
    exit(1);
  }

  fflush(stdout);

  return (nargs);
}

/*!
 \fn int parseCommandLine(int argc, char **argv)
 \brief Parses the command-line
 \param   argc  number of command line arguments
 \param   argv  pointer to a character pointer
 \param      P  reference to parameters
 \returns       if all necessary parameters were set
 */
static bool parseCommandLine(int argc, char *argv[], Parameters & P)
{
  int nargs;
  int inputargs = argc;
  for (; argc > 0 && ISOPTION(*argv[0]); argc--, argv++)
  {
    nargs = parseNextCommand(argc, argv, P);
    argc -= nargs;
    argv += nargs;
  }

  if (inputargs == 0)
  {
    printUsage();
    exit(1);
  }

/*  bool test1 = (P.mov != "" && P.dst != "" && P.lta != "");
  if (!test1)
  {
    printUsage();
    cerr << endl << endl << "ERROR: Please specify --mov --dst and --lta !  "
        << endl << endl;
    exit(1);
  }
  bool test2 = (P.satit || P.sat > 0 || P.cost != Registration::ROB
      || P.leastsquares);
  if (!test2)
  {
    printUsage();
    cerr << endl << endl
        << "ERROR: Please specify either --satit or --sat <float> !  " << endl
        << endl;
    exit(1);
  }
  bool test3 = (P.iscaleout == "" || P.iscale);
  if (!test3)
  {
    printUsage();
    cerr << endl << endl
        << "ERROR: Please specify --iscale together with --iscaleout to compute and output global intensity scaling! "
        << endl << endl;
    exit(1);
  }
  bool test4 = (P.warpout == "" || (P.warpout != P.weightsout));
  if (!test4)
  {
    printUsage();
    cerr << endl << endl
        << "ERROR: Resampled input name (--mapmov) cannot be same as --weights output!"
        << endl << endl;
    exit(1);
  }

  return (test1 && test2 && test3 && test4);*/
  return true;
}


