/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooRealFormula.cc,v 1.2 2001/03/19 15:57:32 verkerke Exp $
 * Authors:
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   07-Mar-2001 WV Created initial version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/

#include "RooFitCore/RooRealFormula.hh"
#include "RooFitCore/RooStreamParser.hh"

ClassImp(RooRealFormula)


RooRealFormula::RooRealFormula(const char *name, const char *title, RooArgSet& dependents) : 
  RooAbsReal(name,title), _formula(name,title,dependents)
{  
  _formula.actualDependents().print() ;

  TIterator* depIter = _formula.actualDependents().MakeIterator() ;
  RooAbsArg* server(0) ;
  while (server=(RooAbsArg*)depIter->Next()) {
    addServer(*server) ;
  }
  setValueDirty(kTRUE) ;
  setShapeDirty(kTRUE) ;
}


RooRealFormula::RooRealFormula(const RooRealFormula& other) : 
  RooAbsReal(other), _formula(other._formula)
{
}


RooRealFormula::~RooRealFormula() 
{
}


Double_t RooRealFormula::evaluate() const
{
  // Evaluate embedded formula
  return _formula.eval() ;
}


Bool_t RooRealFormula::isValid() const
{
  return isValid(getVal()) ;
}


Bool_t RooRealFormula::setFormula(const char* formula) 
{
  if (_formula.reCompile(formula)) return kTRUE ;
  
  SetTitle(formula) ;
  setValueDirty(kTRUE) ;
  return kFALSE ;
}



Bool_t RooRealFormula::isValid(Double_t value) const {
  return kTRUE ;
}


Bool_t RooRealFormula::redirectServersHook(RooArgSet& newServerList, Bool_t mustReplaceAll)
{
  // Propagate server change to formula engine
  return _formula.changeDependents(newServerList,mustReplaceAll) ;
}


void RooRealFormula::printToStream(ostream& os, PrintOption opt) const
{
  // Print current value and definition of formula
  os << "RooRealFormula: " << GetName() << " = " << GetTitle() << " = " << getVal();
  if(!_unit.IsNull()) os << ' ' << _unit;
  printAttribList(os) ;
  os << endl ;
} 


Bool_t RooRealFormula::readFromStream(istream& is, Bool_t compact, Bool_t verbose)
{
  if (compact) {
    cout << "RooRealFormula::readFromStream(" << GetName() << "): can't read in compact mode" << endl ;
    return kTRUE ;
  } else {
    RooStreamParser parser(is) ;
    return setFormula(parser.readLine()) ;
  }
}


void RooRealFormula::writeToStream(ostream& os, Bool_t compact) const
{
  if (compact) {
    cout << "RooRealFormula::writeToStream(" << GetName() << "): can't write in compact mode" << endl ;
  } else {
    os << GetTitle() ;
  }
}



