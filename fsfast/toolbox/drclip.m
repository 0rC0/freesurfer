function [y, xthresh, nclip, psqueeze] = drclip(x,thr,clipval)
%
% [y, xthresh, nclip, psqueeze] = drclip(x,thr,<clipval>)
%
% Squeezes dynamic range by clipping a fraction (thr) of the
% elements of greatest value to clipval.
%
%
%


%
% drclip.m
%
% Original Author: Doug Greve
% CVS Revision Info:
%    $Author: nicks $
%    $Date: 2007/01/10 22:02:30 $
%    $Revision: 1.2 $
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

if(nargin ~= 2 & nargin ~= 3)
  msg = 'USAGE: [y, xthresh, nclip, psqueeze] = drclip(x,thr,<clipval>)';
  qoe(msg); error(msg);
end

if(nargin == 2) clipval = 0; end

xsize = size(x);

x = reshape1d(x);
nx = length(x);
nb = floor(nx/20);
if(nb > 200) nb = 200; end

xmin = min(x);
xmax = max(x);

[xhist xbin] = hist(x,nb);

cdf = cumsum((xhist/nx));

ithresh = min(find(cdf>thr));
xthresh = xbin(ithresh);

isupthresh = find(x>xthresh);
y = x;
y(isupthresh) = clipval;
y = reshape(y,xsize);

nclip = length(isupthresh);
psqueeze = (xthresh-xmin)/(xmax-xmin);

return;
