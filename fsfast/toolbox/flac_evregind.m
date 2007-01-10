function evregind = flac_evregind(flac,nthev)
% evregind = flac_evregind(flac,nthev);
%
% Returns the columns of the regressors in the full design matrix for
% the given EV.
%
%


%
% flac_evregind.m
%
% Original Author: Doug Greve
% CVS Revision Info:
%    $Author: nicks $
%    $Date: 2007/01/10 22:02:32 $
%    $Revision: 1.3 $
%
% Copyright (C) 2002-2007,
% The General Hospital Corporation (Boston, MA). 
% All rights reserved.
%
% Distribution, usage and copying of this software is covered under the
% terms found in the License Agreement file named 'COPYING' found in the
% FreeSurfer source code root directory, and duplicated here:
% https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
%
% General inquiries: freesurfer@nmr.mgh.harvard.edu
% Bug reports: analysis-bugs@nmr.mgh.harvard.edu
%

evregind = [];
if(nargin ~= 2)
  fprintf('evregind = flac_evregind(flac,nthev);\n');
  return;
end

nev = length(flac.ev);
if(nthev > nev) 
  fprintf('ERROR: requested nthEV=%d > nEVs=%d\n',nthev,nev);
  return;
end

nregcum = 1;
for mthev = 1:nthev-1
  nregcum = nregcum + flac.ev(mthev).nreg;
end

evregind = [nregcum:nregcum+flac.ev(nthev).nreg-1];

return;

