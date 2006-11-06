function pdfx = pdf_gamma(x,a,b)
% pdfx = pdf_gamma(x,a,b)
% Gamma distribution evaluated at x for parameters a and b
%
% pdfx = (b.^2) .* (x.^(a-1)) .* exp(-b.*x) ./ gamma(a);
% mean = a/b
% var  = a/(b^2)
% mode = (a-1)/b --> this the value of x at the peak
%
% When creating a hemodynamic response (eg, as in FSL), then
%   a = (tm/sigma)^2
%   b = tm/(sigma^2)
% where sigma = "Stddev" and tm = "Mean lag"
%
% $Id: pdf_gamma.m,v 1.3 2006/11/06 06:01:27 greve Exp $

pdfx = [];

if(nargin ~= 3)
  fprintf('pdfx = pdf_gamma(x,a,b)\n');
  return;
end

nx = length(x);
pdfx = zeros(nx,1);
ind = find(x>0);
pdfx(ind) = (b.^2) .* (x(ind).^(a-1)) .* exp(-b.*x(ind)) ./ gamma(a);

return;



