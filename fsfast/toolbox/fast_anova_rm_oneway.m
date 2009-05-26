function [X C] = fast_anova_rm_oneway(Ns,Nr,SubjFastest)
% [X C] = fast_anova_rm_oneway(Ns,Nr,<RepFastest>)
%
% Design and contrast matrices for a One-way Repeated Measures
% ANOVA with Ns subjects and Nr replicants 
%
% By default, it is assumed that the data are
% arranged in y such that the first Nr rows are the replicants of
% subject 1, the next Nr rows are the replicants of subject 2,
% etc, ie, the replicant is "fastest". If SubjFastest=1, the
% subject is assumed to be fastest, ie, the first Ns rows are
% the first replicant of all subjects, the next Ns rows are
% the second replicant of all subjects, etc.
% Note: When Nr=1, this reduces to an unsigned paired t.
%
% X is the (Ns*Nr)-by-(Ns+Nr-1) design matrix
% C is the (Nr-1)-by-(Ns+Nr-1) contrast matrix
%
%    [beta, rvar, vdof, r] = fast_glmfit(y,X);
%    [F, Fsig, con] = fast_fratio(beta,X,rvar,C);
%
% $Id: fast_anova_rm_oneway.m,v 1.4 2009/05/26 22:36:37 greve Exp $

X = [];
C = [];
if(nargin ~= 2 & nargin ~= 3)
  fprintf('[X C] = fast_anova_rm_oneway(Ns,Nr,<SubjFastest>)\n');
  return;
end

if(nargin == 2) SubjFastest = 0; end 

% Number of data points
Ny = Ns*Nr;

% Create a matrix to model the subject means
Xsubj = fast_blockdiag(ones(Nr,1,Ns));

% Create a matrix to model the difference between replicants
XrepDiff = [];
for nthRep = 1:Nr-1
  M = zeros(Nr,1);
  M(1) = 1;
  M(nthRep+1) = -1;
  XM = repmat(M,[Ns 1]);
  XrepDiff = [XrepDiff XM];
end

% Final design matrix
X = [Xsubj XrepDiff];

% Contrast matrix tests for a difference between replicants
C = [zeros(Nr-1,Ns) eye(Nr-1)];

if(SubjFastest)
  ind = [];
  for nthRep = 1:Nr
    ind = [ind nthRep:Nr:Ny];
  end
  X = X(ind,:);
  % C stays the same
end



return;

