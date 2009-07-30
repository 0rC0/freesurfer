/**
 * @file  LayerEditable.h
 * @brief Base Layer class for editable volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2009/07/30 00:35:50 $
 *    $Revision: 1.4.2.4 $
 *
 * Copyright (C) 2008-2009,
 * The General Hospital Corporation (Boston, MA).
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */

#ifndef LayerEditable_h
#define LayerEditable_h

#include "Layer.h"
#include "vtkSmartPointer.h"
#include <string>

class LayerEditable : public Layer
{
public:
  LayerEditable();
  virtual ~LayerEditable();

  virtual bool HasUndo()
  {
    return false;
  }
  virtual bool HasRedo()
  {
    return false;
  }

  virtual void Undo()
  {}
  virtual void Redo()
  {}

  virtual void Append2DProps( vtkRenderer* renderer, int nPlane ) = 0;
  virtual void Append3DProps( vtkRenderer* renderer, bool* bSliceVisibility = NULL ) = 0;

  virtual void SetVisible( bool bVisible = true ) = 0;
  virtual bool IsVisible() = 0;

  void ResetModified()
  {
    m_bModified = false;
  }

  bool IsModified()
  {
    return m_bModified;
  }

  void SetEditable( bool bEditable = true )
  {
    m_bEditable = bEditable;
  }

  bool IsEditable()
  {
    return m_bEditable;
  }

  const char* GetFileName()
  {
    return m_sFilename.c_str();
  }

  void SetFileName( const char* fn )
  {
    m_sFilename = fn;
  }

  const char* GetRegFileName()
  {
    return m_sRegFilename.c_str();
  }

  void SetRegFileName( const char* fn )
  {
    m_sRegFilename = fn;
  }

protected:
  virtual void SetModified();

  int   m_nMaxUndoSteps;
  bool  m_bModified;

  bool   m_bEditable;

  std::string m_sFilename;
  std::string m_sRegFilename;
};

#endif


