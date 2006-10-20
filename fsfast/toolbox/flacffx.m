function [Fsig, F, betamn] = flacffx(flac,conname,saveresults,jknthrun)
% [Fsig F beta] = flacffx(flac,<conname>,<saveresults>,<jknthrun>)
%
% Fixed effects averaging of the first-level analysis.
%
% flac is a customized flac.
% conname is name of a contrast or empty for all.
% saveresults=1 to save results in flacname/ffx
% jknthrun to jackknife the run. If saveresults=1,
%   then the results are saved in flacname/ffx-RRR
%
% Outputs are all mristructs:
%  Fsig is -log10(p). 
%  F is the F ratio. 
%  beta is mean beta over runs.
%
% If there are multiple contrasts, Fsig and F are for the last ones.
%
% If saveresults, 
%
% $Id: flacffx.m,v 1.10 2006/10/20 05:20:48 greve Exp $
%

Fsig = [];
F = [];
betamn = [];
if(nargin < 1)
  fprintf('[Fsig F beta] = flacffx(flac,<conname>,<saveresults>,<jknthrun>)\n');
  return;
end

if(~exist('conname','var')) conname = ''; end
if(~exist('saveresults','var')) saveresults = 0; end
if(~exist('jknthrun','var')) jknthrun = 0; end

nruns = size(flac.runlist,1);
if(jknthrun > nruns) 
  fprintf('ERROR: jknthrun = %d > nruns = %d\n',jknthrun,nruns) ;
  return;
end

if(jknthrun == 0) nruns_use = nruns;
else              nruns_use = nruns-1;
end

ncon = length(flac.con);
if(isempty(conname)) conind = 1:ncon;
else
  conind = flac_conindex(conname,flac);
  if(isempty(conind))
    fprintf('ERROR: could not find contrast %s in flac\n',conname);
    return;
  end
end

if(saveresults)
  flaffxdir = sprintf('%s/%s/%s',flac.sess,flac.fsd,flac.name);
  if(jknthrun ~= 0)
    flaffxdir = sprintf('%s/jk%s',flaffxdir,flac.runlist(jknthrun,:));
  end
  mkdirpcmd = sprintf('mkdir -p %s',flaffxdir);
  unix(mkdirpcmd);
end

% Start loop over runs -------------------%
dof = 0;
jthrun = 0;
for nthrun = 1:nruns

  % Skip if this is a jackknifed run
  if(nthrun == jknthrun) 
    %fprintf('  nthrun = %d/%d -- jackknifing\n',nthrun,nruns);
    continue; 
  end
  jthrun = jthrun + 1;

  % Customize the flac
  %fprintf('  nthrun = %d/%d\n',nthrun,nruns);
  flac.nthrun = nthrun;
  flac = flac_customize(flac);

  dofrun = size(flac.X,1) - size(flac.X,2);
  dof = dof + dofrun;
  
  % Load the betas for this run
  betarun = MRIread(flac.betafspec);
  if(isempty(betarun))
    fprintf('ERROR: loading %s\n',flac.betafspec);
    return;
  end
  
  if(jthrun == 1) 
    betamn = betarun;
    betamn.vol = 0;
  end
  
  % Keep a running average - not used in FFX
  betamn.vol  = betamn.vol + betarun.vol; 

  % Reshape beta
  betarun.vol = fast_vol2mat(betarun.vol);
  
  % Load the residual variances
  rvarrun = MRIread(flac.rvarfspec);

  % Compute and accumulate sum of squares
  ssrrun = dofrun*rvarrun.vol;
  if(jthrun == 1)
    ssr = rvarrun;
    ssr.vol = 0;
  end
  ssr.vol = ssr.vol + ssrrun;

  % Load the matfile to get info about the temporal cor
  fladir = sprintf('%s/%s/%s/%s',...
		   flac.sess,flac.fsd,flac.name,flac.runlist(nthrun,:));
  matfile = sprintf('%s/flac.mat',fladir);
  flacproc = load(matfile);
  nseg = size(flacproc.nacfseg,2);
  
  % Go through each contrast
  for nthcon = conind
    C = flac.con(nthcon).C;
    J = size(C,1);

    % Compute gamma
    gamrun = C*betarun.vol;
    if(jthrun == 1) % jk
      gam(nthcon) = betarun; 
      gam(nthcon).vol = 0;
    end
    % Accumulate (need to divide by nruns)
    gam(nthcon).vol = gam(nthcon).vol + gamrun;

    % Compute the variance reduction factor or matrix
    for nthseg = 1:nseg+1
      if(nthseg == 1) S = eye(flac.ntp);
      else
	nacf = flacproc.nacfseg(:,nthseg-1);
	S = toeplitz(nacf);
      end
      cixtxctrun = C*inv(flac.X'*inv(S)*flac.X)*C';
      if(jthrun == 1) % jk
	flac.con(nthcon).cixtxct(:,:,nthseg) = zeros(J); 
      end
    % Accumulate (need to divide by nruns)
      flac.con(nthcon).cixtxct(:,:,nthseg) = ...
	  flac.con(nthcon).cixtxct(:,:,nthseg) + cixtxctrun;
    end % seg
  
  end % con
  
end % run

% Divide by number of runs
rvar = ssr;
rvar.vol = ssr.vol/dof;
betamn.vol  = betamn.vol/nruns_use;
for nthcon = conind
  gam(nthcon).vol = gam(nthcon).vol/nruns_use;
  flac.con(nthcon).cixtxct = flac.con(nthcon).cixtxct/(nruns_use.^2);
end

if(saveresults)
  outfspec = sprintf('%s/beta.%s',flaffxdir,flac.format);
  MRIwrite(betamn,outfspec);
  outfspec = sprintf('%s/rvar.%s',flaffxdir,flac.format);
  MRIwrite(rvar,outfspec);
  rstd = rvar;
  rstd.vol = sqrt(rvar.vol);
  outfspec = sprintf('%s/rstd.%s',flaffxdir,flac.format);
  MRIwrite(rstd,outfspec);

  % Save the baseline separately
  blevind = flac_evindex(flac,'Baseline');
  if(~isempty(blevind))
    fprintf('Saving baseline\n');
    blregind = flac_evregind(flac,blevind);
    tmp = rvar;
    tmp.vol = betamn.vol(:,:,:,blregind);
    stem = sprintf('%s/h-offset%s',flaffxdir,flac.formatext);
    MRIwrite(tmp,stem);
  end

end

% Now go through each contrast and compute the significances
for nthcon = conind
  %fprintf('  contrast %s %d/%d\n',flac.con(nthcon).name,nthcon,ncon);
  C = flac.con(nthcon).C;
  J = size(C,1);
  F = gam(nthcon);
  nvox = size(gam(nthcon).vol,1);
  F.vol = zeros(1,nvox);
  cescvm = gam(nthcon);
  cescvm.vol = zeros(J*J,nvox);
  for nthseg = 1:nseg+1
    indseg = find(flacproc.acfseg.vol == nthseg-1);
    cixtxct = flac.con(nthcon).cixtxct(:,:,nthseg);
    inv_cixtxct = inv(cixtxct);
    tmp = (gam(nthcon).vol(:,indseg)' * inv_cixtxct)';
    if(J > 1)
      tmp = sum(tmp .* gam(nthcon).vol(:,indseg));
    else
      tmp = tmp .* gam(nthcon).vol(:,indseg);
    end      
    F.vol(indseg) = tmp./(J*rvar.vol(indseg)');
    cescvm.vol(:,indseg) = reshape1d(cixtxct) * rvar.vol(indseg)'; %outerproduct 
  end % seg
  p = F;
  p.vol = FTest(J, dof, F.vol);
  ind = find(p.vol == 0);
  p.vol(ind) = eps;
  Fsig = p;
  Fsig.vol = -log10(p.vol);

  if(J == 1)
    F.vol    = F.vol    .* sign(gam(nthcon).vol);
    Fsig.vol = Fsig.vol .* sign(gam(nthcon).vol);
  end

  F.vol    = fast_mat2vol(F.vol,F.volsize);
  p.vol    = fast_mat2vol(p.vol,p.volsize);
  Fsig.vol = fast_mat2vol(Fsig.vol,Fsig.volsize);
      
  if(saveresults)
    condir = sprintf('%s/%s',flaffxdir,flac.con(nthcon).name);
    mkdirpcmd = sprintf('mkdir -p %s',condir);
    unix(mkdirpcmd);

    outfspec = sprintf('%s/f.%s',condir,flac.format);
    MRIwrite(F,outfspec);
  
    outfspec = sprintf('%s/sig.%s',condir,flac.format);
    MRIwrite(Fsig,outfspec);
  
    outfspec = sprintf('%s/ces.%s',condir,flac.format);
    tmp = gam(nthcon);
    tmp.vol = fast_mat2vol(tmp.vol,tmp.volsize);
    MRIwrite(tmp,outfspec);
  
    outfspec = sprintf('%s/cesvar.%s',condir,flac.format);
    tmp = cescvm;
    tmp.vol = fast_mat2vol(tmp.vol,tmp.volsize);
    MRIwrite(tmp,outfspec);
  
  end

end % con

%fprintf('  flacffx done\n');



