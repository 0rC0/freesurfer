function [err, racfexp] = fast_ar1w_fiterr(p,racf,R,w)
% err = fast_ar1wfiterr(p,racf,R,<w>)
%
% error function for fitting an AR1+W noise model while also taking
% into account the bias introduced by GLM fitting. This function
% can be used with fminsearch, which uses Nelder-Mead 
% unconstrained non-linear search. 'Unconstrained' can cause
% problems. 
%
% p = [alpha rho];
% racf is actual residual ACF (nf-by-1). Should be created with
%       racf = fast_acorr(r,'unbiasedcoeff');
% R is residual forming matrix (nf-by-nf)
% w is the weight of each delay (nf-by-1). If unspecified,
%   no weighting is performed. In simulations, works best
%   without weighting. But those are simulations. Consider
%   weighting with w = 1./[1:nf]';
% 
% err = sum( abs(racf-racfexp).*w ); % L1 norm
% 
% Autocorrelation function of AR1+White Noise
%   acf(0) = 1
%   acf(n) = (1-alpha)*rho^n
%
% Example:
%  p = [0 racf(2)]; % Init as simple AR1
%  popt = fminsearch('fast_ar1w_fiterr',p,[],racf,R);
%  nacfopt = fast_ar1w_acf(popt(1),popt(2),nf);
%  [e racfexp] = fast_ar1w_fiterr(popt,racfmn,R);
%  plot(1:nf,racf,1:nf,racfexp)
% 
% Notes: 
%  1. For nf=100 and nice racfs, takes about .35 sec on 
%     Athalon 1GHz and scales with nf^2.
%  2. Unconstrained search can yield alpha<0 and rho > 1.
%  3. As alpha->1, the error is less sensitive to rho.
%  4. There is a danger of local minima. This parameterization
%     is not well conditioned as very different combinations
%     of alpha and rho can lead to very similar ACFs. Try
%     initializing with fast_ar1w_fit on the RACF.
%
% See also: fast_ar1w_acf, fast_acorr, fast_ar1w_fit.
%
% $Id: fast_ar1w_fiterr.m,v 1.2 2004/04/29 22:22:20 greve Exp $
%
% (c) Douglas N. Greve, 2004
%

% Number of frames
nf = length(racf);

% Create ideal noise ACF based on AR1+W parameters
alpha = p(1);
rho   = p(2);
nacf = fast_ar1w_acf(alpha,rho,nf);

% Create the noise covariaance matrix
Sn = toeplitz(nacf);

% Create the expected residual covariance matrix
%Srexp = R*Sn*R;
% Extract the expected residual ACF
%racfexp = fast_cvm2acor(Srexp);

% But this is about 4X faster
racfexp = (R(1,:)*Sn*R)';
racfexp = racfexp/racfexp(1);

% Error is L1 difference between actual and expected
if(exist('w','var'))
  err = sum( abs(racf-racfexp).*w );
else
  err = sum( abs(racf-racfexp) );
end

return;

