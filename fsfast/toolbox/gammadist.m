function y = gammadist(x,avg,stddev)
% Gamma Distribution
%
% y = gammadist(x,avg,stddev)
%
% This probably does not work.


%
% gammadist.m
%
% Original Author: Doug Greve
% CVS Revision Info:
%    $Author: nicks $
%    $Date: 2007/01/10 22:02:34 $
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

if(nargin ~= 3) 
  msg = 'USAGE: y = gammadist(x,avg,stddev)'
  error(msg);
end

z = (x-avg)/stddev;
y = (z.^2) .* exp(-z);
ind = find(z<0);
y(ind) = 0;
y = y/sum(y);

return
