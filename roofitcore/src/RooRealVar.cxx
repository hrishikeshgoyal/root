/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooRealVar.cc,v 1.6 2001/03/28 00:21:52 verkerke Exp $
 * Authors:
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   07-Mar-2001 WV Created initial version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "TObjString.h"
#include "TTree.h"
#include "RooFitCore/RooRealVar.hh"
#include "RooFitCore/RooStreamParser.hh"

ClassImp(RooRealVar)


RooRealVar::RooRealVar(const char *name, const char *title,
		       Double_t value, const char *unit) :
  RooAbsReal(name, title, 0, 0, unit), _error(0),
  _fitMin(-1e10), _fitMax(1e10)
{
  _value = value ;
  setConstant(kTRUE) ;
  setValueDirty(kTRUE) ;
  setShapeDirty(kTRUE) ;
}  

RooRealVar::RooRealVar(const char *name, const char *title,
		       Double_t minValue, Double_t maxValue,
		       const char *unit) :
  RooAbsReal(name, title, minValue, maxValue, unit),
  _fitMin(minValue), _fitMax(maxValue)

{
  _value= 0.5*(minValue + maxValue);
  setValueDirty(kTRUE) ;
  setShapeDirty(kTRUE) ;
}  

RooRealVar::RooRealVar(const char *name, const char *title,
		       Double_t value, Double_t minValue, Double_t maxValue,
		       const char *unit) :
  RooAbsReal(name, title, minValue, maxValue, unit), _error(0),
  _fitMin(minValue), _fitMax(maxValue)
{
  _value = value ;
  setValueDirty(kTRUE) ;
  setShapeDirty(kTRUE) ;
}  

RooRealVar::RooRealVar(const RooRealVar& other) :
  RooAbsReal(other), 
  _error(other._error),
  _fitMin(other._fitMin),
  _fitMax(other._fitMax)
{
  setConstant(other.isConstant()) ;
  setProjected(other.isProjected()) ;
}

RooRealVar::~RooRealVar() 
{
}

RooRealVar::operator Double_t&() {
  return _value;
}

RooRealVar::operator Double_t() const {
  return this->getVal();
}


void RooRealVar::setVal(Double_t value) {

  // Set current value
  Double_t clipValue ;
  inFitRange(value,&clipValue) ;

  setValueDirty(kTRUE) ;
  _value = clipValue;
}


void RooRealVar::setFitMin(Double_t value) 
{
  // Check if new limit is consistent
  if (_fitMin>_fitMax) {
    cout << "RooRealVar::setFitMin(" << GetName() 
	 << "): Proposed new fit min. larger than max., setting min. to max." << endl ;
    _fitMin = _fitMax ;
  } else {
    _fitMin = value ;
  }

  // Clip current value in window if it fell out
  Double_t clipValue ;
  if (!inFitRange(_value,&clipValue)) {
    setVal(clipValue) ;
  }

  setShapeDirty(kTRUE) ;
}


void RooRealVar::setFitMax(Double_t value)
{
  // Check if new limit is consistent
  if (_fitMax<_fitMin) {
    cout << "RooRealVar::setFitMax(" << GetName() 
	 << "): Proposed new fit max. smaller than min., setting max. to min." << endl ;
    _fitMax = _fitMax ;
  } else {
    _fitMax = value ;
  }

  // Clip current value in window if it fell out
  Double_t clipValue ;
  if (!inFitRange(_value,&clipValue)) {
    setVal(clipValue) ;
  }

  setShapeDirty(kTRUE) ;
}


void RooRealVar::setFitRange(Double_t min, Double_t max) {
  // Check if new limit is consistent
  if (min>max) {
    cout << "RooRealVar::setFitRange(" << GetName() 
	 << "): Proposed new fit max. smaller than min., setting max. to min." << endl ;
    _fitMin = min ;
    _fitMax = min ;
  } else {
    _fitMin = min ;
    _fitMax = max ;
  }

  setShapeDirty(kTRUE) ;  
}



Double_t RooRealVar::operator=(Double_t newValue) 
{
  // Clip 
  inFitRange(newValue,&_value) ;

  setValueDirty(kTRUE) ;
  return _value;
}



Bool_t RooRealVar::inFitRange(Double_t value, Double_t* clippedValPtr) const
{
  // Check which limit we exceeded and truncate. Print a warning message
  // unless we are very close to the boundary.  
  
  Double_t range = _fitMax - _fitMin ;
  Double_t clippedValue(value);
  Bool_t inRange(kTRUE) ;

  if (hasFitLimits()) {
    if(value > _fitMax) {
      if(value - _fitMax > 1e-6*range) {
	if (clippedValPtr)
	  cout << "RooRealVar::inFitRange(" << GetName() << "): value " << value
	       << " rounded down to max limit " << _fitMax << endl;
      }
      clippedValue = _fitMax;
      inRange = kFALSE ;
    }
    else if(value < _fitMin) {
      if(_fitMin - value > 1e-6*range) {
	if (clippedValPtr)
	  cout << "RooRealVar::inFitRange(" << GetName() << "): value " << value
	       << " rounded up to min limit " << _fitMin << endl;
      }
      clippedValue = _fitMin;
      inRange = kFALSE ;
    } 
  }

  if (clippedValPtr) *clippedValPtr=clippedValue ;
  return inRange ;
}




Bool_t RooRealVar::isValid() const
{
  return isValid(getVal()) ;
}


Bool_t RooRealVar::isValid(Double_t value, Bool_t verbose) const {
  if (!inFitRange(value)) {
    if (verbose)
      cout << "RooRealVar::isValid(" << GetName() << "): value " << value << " out of range" << endl ;
    return kFALSE ;
  }
  return kTRUE ;
}



void RooRealVar::attachToTree(TTree& t, Int_t bufSize)
{
  // Attach object to a branch of given TTree

  // First determine if branch is taken
  if (t.GetBranch(GetName())) {
    //cout << "RooRealVar::attachToTree(" << GetName() << "): branch in tree " << t.GetName() << " already exists" << endl ;
    t.SetBranchAddress(GetName(),&_value) ;
  } else {    
    TString format(GetName());
    format.Append("/D");
    t.Branch(GetName(), &_value, (const Text_t*)format, bufSize);
  }
}


Bool_t RooRealVar::readFromStream(istream& is, Bool_t compact, Bool_t verbose) 
{
  // Read object contents from given stream
  TString token,errorPrefix("RooRealVar::readFromStream(") ;
  errorPrefix.Append(GetName()) ;
  errorPrefix.Append(")") ;
  RooStreamParser parser(is,errorPrefix) ;
  Double_t value(0) ;

  if (compact) {
    // Compact mode: Read single token
    if (parser.readDouble(value,verbose)) return kTRUE ;
    if (isValid(value,kTRUE)) {
      setVal(value) ;
      return kFALSE ;
    } else {
      return kTRUE ;
    }

  } else {
    // Extended mode: Read multiple tokens on a single line   
    Bool_t haveValue(kFALSE) ;
    while(1) {      
      if (parser.atEOL()) break ;
      token=parser.readToken() ;

      if (!token.CompareTo("+/-")) {

	// Next token is error
	Double_t error ;
	if (parser.readDouble(error)) break ;
	setError(error) ;

      } else if (!token.CompareTo("C")) {

	// Set constant
	setConstant(kTRUE) ;

      } else if (!token.CompareTo("P")) {

	// Next tokens are plot limits
	Double_t plotMin, plotMax ;
        Int_t plotBins ;
	if (parser.expectToken("(",kTRUE) ||
	    parser.readDouble(plotMin,kTRUE) ||
	    parser.expectToken("-",kTRUE) ||
	    parser.readDouble(plotMax,kTRUE) ||
            parser.expectToken(":",kTRUE) ||
            parser.readInteger(plotBins,kTRUE) || 
	    parser.expectToken(")",kTRUE)) break ;
	setPlotRange(plotMin,plotMax) ;

      } else if (!token.CompareTo("F")) {

	// Next tokens are fit limits
	Double_t fitMin, fitMax ;
	if (parser.expectToken("(",kTRUE) ||
	    parser.readDouble(fitMin,kTRUE) ||
	    parser.expectToken("-",kTRUE) ||
	    parser.readDouble(fitMax,kTRUE) ||
	    parser.expectToken(")",kTRUE)) break ;
	setFitRange(fitMin,fitMax) ;
      } else {
	// Token is value
	if (parser.convertToDouble(token,value)) { parser.zapToEnd() ; break ; }
	haveValue = kTRUE ;
	// Defer value assignment to end
      }
    }    
    if (haveValue) setVal(value) ;
    return kFALSE ;
  }
}


void RooRealVar::writeToStream(ostream& os, Bool_t compact) const
{
  // Write object contents to given stream

  if (compact) {
    // Write value only
    os << getVal() ;
  } else {
    // Write value
    os << getVal() << " " ;
  
    // Append error if non-zero 
    Double_t err = getError() ;
    if (err!=0) {
      os << "+/- " << err << " " ;
    }
    // Append limits if not constants
    if (isConstant()) {
      os << "C " ;
    }      
    // Append plot limits
    os << "P(" << getPlotMin() << " - " << getPlotMax() << " : " << getPlotBins() << ") " ;      
    // Append fit limits if not +Inf:-Inf
    if (hasFitLimits()) {
      os << "F(" << getFitMin() << " - " << getFitMax() << ") " ;      
    }
    // Add comment with unit, if unit exists
    if (!_unit.IsNull())
      os << "// [" << getUnit() << "]" ;
  }
}



RooAbsArg&
RooRealVar::operator=(RooAbsArg& aorig)
{
  // Assignment operator for RooRealVar
  RooAbsReal::operator=(aorig) ;

  RooRealVar& orig = (RooRealVar&)aorig ;
  _error = orig._error ;
  _fitMin = orig._fitMin ;
  _fitMax = orig._fitMax ;

  return (*this) ;
}

void RooRealVar::printToStream(ostream& os, PrintOption opt) const {
  switch(opt) {
  case Verbose:
    os << fName << " = " << getVal() << " +/- " << _error;    
    if(!_unit.IsNull()) os << ' ' << _unit;
    printAttribList(os) ;
    os << endl;
    break ;
    
  case Shape:
    os << fName << ": " << fTitle;
    if(isConstant()) {
      os << ", fixed at " << getVal();
    }
    else {
      os << ", range is (" << _fitMin << "," << _fitMax << ")";
    }
    if(!_unit.IsNull()) os << ' ' << _unit;
    printAttribList(os) ;
    os << endl;
    break ;
    
  case Standard:
    os << "RooRealVar: " << GetName() << " = " << getVal();
    if(!_unit.IsNull()) os << ' ' << _unit;
    os << " : " << GetTitle() ;
    if(!isConstant() && hasFitLimits())
      os << " (" << _fitMin << ',' << _fitMax << ')';
    else if (isConstant()) 
      os << " Constant" ;
    os << endl ;	
    break ;
  }
}


TString *RooRealVar::format(Int_t sigDigits, const char *options) {
  // Format numeric value in a variety of ways

  // parse the options string
  TString opts(options);
  opts.ToLower();
  Bool_t showName= opts.Contains("n");
  Bool_t hideValue= opts.Contains("h");
  Bool_t showError= opts.Contains("e");
  Bool_t showUnit= opts.Contains("u");
  Bool_t tlatexMode= opts.Contains("l");
  Bool_t latexMode= opts.Contains("x");
  Bool_t useErrorForPrecision=
    (showError && !isConstant()) || opts.Contains("p");
  // calculate the precision to use
  if(sigDigits < 1) sigDigits= 1;
  Double_t what= (useErrorForPrecision) ? _error : _value;
  Int_t leadingDigit= (Int_t)floor(log10(fabs(what)));
  Int_t where= leadingDigit - sigDigits + 1;
  char fmt[16];
  sprintf(fmt,"%%.%df", where < 0 ? -where : 0);
  TString *text= new TString();
  if(latexMode) text->Append("$");
  // begin the string with "<name> = " if requested
  if(showName) {
    text->Append(getPlotLabel());
    text->Append(" = ");
  }
  // append our value if requested
  char buffer[256];
  if(!hideValue) {
    Double_t chopped= chopAt(_value, where);
    sprintf(buffer, fmt, _value);
    text->Append(buffer);
  }
  // append our error if requested and this variable is not constant
  if(!isConstant() && showError) {
    if(tlatexMode) {
      text->Append(" #pm ");
    }
    else if(latexMode) {
      text->Append("\\pm ");
    }
    else {
      text->Append(" +/- ");
    }
    sprintf(buffer, fmt, _error);
    text->Append(buffer);
  }
  // append our units if requested
  if(!_unit.IsNull() && showUnit) {
    text->Append(' ');
    text->Append(_unit);
  }
  if(latexMode) text->Append("$");
  return text;
}

Double_t RooRealVar::chopAt(Double_t what, Int_t where) {
  // What does this do?
  Double_t scale= pow(10.0,where);
  Int_t trunc= (Int_t)floor(what/scale + 0.5);
  return (Double_t)trunc*scale;
}

