function y = trapezoid(t,Tru,Tft,Trd,Td)
% y = trapezoid(t,Tru,Tft,Trd,<Td>)
% 
% Computes the value of the trapezoid waveform at time t
% t can be a vector.
%
% Tru = Ramp Up Duration
% Tft = Flattop Duration
% Trd = Ramp Down Duration
%
% Td is an optional delay.
% 
% For times before Td and after the end, the result is 0.
% The amplitude at the flat top is 1.
%
% $Id: trapezoid.m,v 1.2 2003/09/16 07:57:06 greve Exp $

y = [];
if(nargin ~= 4 & nargin ~= 5)
  fprintf('y = trapezoid(t,Tru,Tft,Trd,<Td>)\n');
  return;
end

if(exist('Td') ~= 1) Td = []; end
if(isempty(Td)) Td = 0; end

% Subtract delay
t = t - Td;

tFTStart = Tru; % Start of Flattop
tRDStart = Tru + Tft; % Start of Ramp Down
tEnd     = Tru + Tft + Trd; % End of Ramp Down

y = zeros(size(t));

%ind = find(t > 0 & t < tFTStart);
ind = find(t <= tFTStart);
if(~isempty(ind)) y(ind) = t(ind)/Tru; end

ind = find(t > tFTStart & t < tRDStart);
if(~isempty(ind)) y(ind) = 1; end

%ind = find(t > tRDStart & t < tEnd);
ind = find(t >= tRDStart );
if(~isempty(ind)) y(ind) = -(t(ind)-tEnd)/Trd; end


return;












