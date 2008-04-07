% fast_selxavg3.m
%
% $Id: fast_selxavg3.m,v 1.58 2008/04/07 21:25:42 greve Exp $


%
% fast_selxavg3.m
%
% Original Author: Doug Greve
% CVS Revision Info:
%    $Author: greve $
%    $Date: 2008/04/07 21:25:42 $
%    $Revision: 1.58 $
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

% Automatically create mask?

% Force choice on whitening or not
% Force choice of mask
% How to check that analyses are not being mixed?
% RFx, FFx, MFx 2nd level analysis
% Func2ROI


if(0)
  monly       = 1;
  DoGLMFit    = 1;
  DoContrasts = 1;
  DoSynth     = 1;
  SynthNoiseAmp  = 1;
  SynthSignalAmp = 1;
  SynthNoiseAR1 = 0.3;
  SynthSeed   = -1;
  analysis = '';
  flacname = '';
  outtop = '';
  
  %sess  = 'tl20000621';
  %flacname = 'flac/edp.flac';
  %analysis = 'edp';
  %analysis = 'main3-fir';
  % outtop = '/space/greve/1/users/greve/workmem-ana';
  
  %sess  = 'dng';
  %flacname = 'flac/rest.flac';
  
  %sess = '/autofs/space/annecy_014/users/kdevaney/subj27/color_orientation_20060403';
  %analysis = 'subj27_color';
  %outtop = '/space/greve/1/users/greve/kd';
end

fprintf('$Id: fast_selxavg3.m,v 1.58 2008/04/07 21:25:42 greve Exp $\n');

if(DoSynth)
  if(SynthSeed < 0) SynthSeed = sum(100*clock); end
  fprintf('SynthSeed     = %10d\n',SynthSeed);
  fprintf('SynthNoiseAmp    = %d\n',SynthNoiseAmp);
  fprintf('SynthNoiseAR1   = %g\n',SynthNoiseAR1);
  fprintf('SynthSignalAmp   = %d\n',SynthSignalAmp);
end

sessname = basename(sess);
%outtop = dirname(sess);
if(isempty(outtop)) outtop = fast_dirname(sess); end
fprintf('outtop = %s\n',outtop);

ext = getenv('FSF_OUTPUT_FORMAT');
if(isempty(ext)) ext = 'bhdr'; end
fprintf('Extension format = %s\n',ext);

fprintf('UseFloat = %d\n',UseFloat);

if(~isempty(analysis))
  flac0 = fast_ldanaflac(analysis);
else
  flac0 = fast_ldflac(flacname);
end
if(isempty(flac0))
  if(~monly) quit; end
  return; 
end

flac0.sess = sess;
flac0.nthrun = 1;
flac0 = flac_customize(flac0);
if(isempty(flac0)) 
  if(~monly) quit; end
  return; 
end

nruns = size(flac0.runlist,1);
fprintf('nruns = %d\n',nruns);
ncontrasts = length(flac0.con);
fprintf('autostimdur = %d\n',flac0.autostimdur);

outanadir0 = sprintf('%s/%s/%s/%s',outtop,sessname,flac0.fsd,flac0.name);

%%%%%%%
if(perrun | jkrun) outer_runlist = [1:nruns];
else outer_runlist = 1;
end

for nthouter = outer_runlist

  outerrun = flac0.runlist(nthouter,:);
  if(perrun)      
    outanadir = sprintf('%s/pr%s',outanadir0,outerrun);
    nthrunlist = nthouter;
  elseif(jkrun)  
    outanadir = sprintf('%s/jk%s',outanadir0,outerrun);
    nthrunlist = setxor(nthouter,[1:nruns]);
  else            
    outanadir = outanadir0;
    nthrunlist = [1:nruns];
  end

  fprintf('\n\n');
  fprintf('outanadir = %s\n',outanadir);
  err = mkdirp(outanadir);
  if(err) return; end

  xfile   = sprintf('%s/X.mat',outanadir);
  outresdir = sprintf('%s/res',outanadir);

  % Load the brain mask
  if(~isempty(flac0.maskfspec))
    mask = MRIread(flac0.maskfspec);
    if(isempty(mask))
      fprintf('ERROR: cannot load %s\n',flac0.maskfspec);
      return;
    end
  else
    tmp = MRIread(flac0.funcfspec,1);
    mask = tmp;
    mask.vol = ones(tmp.volsize);
    clear tmp;
  end

  indmask = find(mask.vol);
  nmask = length(indmask);
  nslices = mask.volsize(3);
  nvox = prod(mask.volsize);
  fprintf('Found %d/%d (%4.1f) voxels in mask\n',nmask,nvox,100*nmask/nvox);
  mri = mask; % save as template
  mri.vol = []; % blank

  % Create a volume with vox val = the slice 
  svol = zeros(mri.volsize);
  for s = 1:nslices,  svol(:,:,s) = s; end

  %---------------------------------------------%
  fprintf('Creating Design Matrix\n');
  Xt = [];
  Xn = [];
  tpindrun = [];
  clear runflac;
  tic;
  for nthrun = nthrunlist
    flac = flac0;
    flac.nthrun = nthrun;
    flac = flac_customize(flac);
    if(isempty(flac)) 
      if(~monly) quit;  end
      return; 
    end
    runflac(nthrun).flac = flac;
    
    indtask = flac_taskregind(flac);
    Xtr = flac.X(:,indtask);
    nTaskRun = size(Xtr,2);
    
    indnuis = flac_nuisregind(flac);
    Xnr = flac.X(:,indnuis);
    nNuisRun = size(Xnr,2);
    
    Za = zeros(size(Xn,1),  size(Xnr,2));
    Zb = zeros(size(Xnr,1), size(Xn,2));
    
    Xt = [Xt; Xtr];
    Xn = [Xn Za; Zb Xnr];
    
    % Keep track of which tps belong to which runs
    tpindrun = [tpindrun; nthrun*ones(flac.ntp,1)];
    
    % Keep track of which regressors are the mean
    if(nthrun == nthrunlist(1)) reg0 = zeros(1,nTaskRun); end
    reg0run = zeros(1,nNuisRun);
    reg0run(1) = 1;
    reg0 = [reg0 reg0run];

    % This is for comparing cross-run FFX analysis vs this analysis 
    % in which the data and design matrices are concatenated.
    Xrun = flac0.X;
    for nthcon = 1:ncontrasts
      C = flac.con(nthcon).C;
      M = C*inv(Xrun'*Xrun)*C';
      if(nthrun == 1) conffx(nthcon).Msum = 0; end
      conffx(nthcon).Msum = conffx(nthcon).Msum + M;
    end

  end
  % These are the true indices of the mean regressors
  ind0 = find(reg0);

  % Create the full design matrix
  X = [Xt Xn];
  nTask = size(Xt,2);
  nNuis = size(Xn,2);
  nX = size(X,2);
  ntptot = size(X,1);
  DOF = ntptot - nX;
  
  doffile = sprintf('%s/dof',outanadir);
  fp = fopen(doffile,'w');
  if(fp == -1)
    fprintf('ERROR: could not open %s\n',doffile);
    return;
  end
  fprintf(fp,'%d\n',DOF);
  fclose(fp);
  fprintf('ntptot = %d, nX = %d, DOF = %d\n',ntptot,nX,DOF);

  % Check condition, normalize to distinguish from scaled
  Xsss = sqrt(sum(X.^2));
  Xn = X ./ repmat(Xsss,[ntptot 1]);
  XtX = Xn'*Xn;
  XCond = cond(XtX);
  fprintf('XCond = %g (normalized)\n',XCond);
  if(XCond > 1e4)
    fprintf('ERROR: design is ill-conditioned\n');
    tmpxfile = sprintf('%s/Xtmp.mat',outanadir);
    fprintf('Saving X matrix to %s\n',tmpxfile);
    save(tmpxfile,'X','flac');
    return;
  end

  B0 = inv(X'*X)*X';
  Ctask = [eye(nTask) zeros(nTask,nNuis)];
  nn = [1:ntptot]';
  R = eye(ntptot) - X*inv(X'*X)*X';
  
  if(flac0.acfbins ~= 0)
    fprintf('Computing compensation for resdual AR1 bias\n');
    [rfm.M rfm.rrho1 rfm.nrho1 rfm.nrho1hat] = fast_rfm2nrho1(R);
    fprintf('AR1 Correction M: %g %g\n',rfm.M(1),rfm.M(2));
  else
    rfm.M(1) = 0;
    rfm.M(2) = 1;
  end

  %---------------------------------------------%
  % The contrast matrices were originally computed assuming
  % only a single run's worth of nuisance regressors. Recompute.
  fprintf('Computing contrast matrices\n');
  flacC = flac0;
  for nthcon = 1:ncontrasts
    indtask = flac_taskregind(flac0);
    C = flacC.con(nthcon).C;
    C = C(:,indtask);
    for nthrun = nthrunlist
      flac = flac_customize(flac0);
      Crun = flac.con(nthcon).C;
      indnuis = flac_nuisregind(flac);    
      Cnuis = Crun(:,indnuis);
      C = [C Cnuis];
    end
    [J K] = size(C);
    flacC.con(nthcon).C = C;
    M = C*inv(X'*X)*C';
    eff = 1/trace(M);
    vrf = 1/mean(diag(M));
    flacC.con(nthcon).M = M;
    flacC.con(nthcon).eff = eff;
    flacC.con(nthcon).vrf = vrf;

    % For FFX-vs-Concat Comparison (see above)
    Mffx = conffx(nthcon).Msum/(flac0.nruns.^2);
    vrfffx = 1/mean(diag(Mffx));

    fprintf('%2d %-10s J=%2d  eff = %6.1f   vrf = %8.4f vrfffx = %8.4f r = %4.2f\n',...
	    nthcon,flac0.con(nthcon).name,J,eff,vrf,vrfffx,vrf/vrfffx);
  end

if(DoGLMFit)
  % Make the output dirs
  err = mkdirp(outanadir);
  if(err) return; end
  err = mkdirp(outresdir);      
  if(err) return; end

  % First pass thru the data to compute beta
  fprintf('OLS Beta Pass \n');
  tic;
  betamat0 = zeros(nX,nvox);
  yrun_randn = [];
  for nthrun = nthrunlist
    fprintf('  run %d    t=%4.1f\n',nthrun,toc);
    flac = flac0;
    flac.nthrun = nthrun;
    flac = flac_customize(flac);
    indrun = find(tpindrun == nthrun);
    if(~DoSynth)
      yrun = MRIread(flac.funcfspec);
      if(abs(yrun.tr/1000 - flac.TR) > .001)
	fprintf('ERROR: TR mismatch between analysis and data\n');
	fprintf('analysis TR = %g, data TR = %g\n',flac.TR,yrun.tr/1000);
	return;
      end
      if(UseFloat) yrun.vol = single(yrun.vol); end
      if(yrun.volsize(1) ~= mask.volsize(1) | ...
	 yrun.volsize(2) ~= mask.volsize(2) | ...
	 yrun.volsize(3) ~= mask.volsize(3))
	fprintf('ERROR: dimension mismatch between mask and %dth run\n',nthrun);
	return;
      end
      yrun = fast_vol2mat(yrun);
    else
      yrun_randn(:,nthrun) = randn('state'); % save state
      ynoise  = 0;
      ysignal = 0;
      if(SynthNoiseAmp > 0)
	ynoise = randn(flac.ntp,nvox);
	if(SynthNoiseAR1 ~= 0)
	  acfsynth = SynthNoiseAR1.^[0:flac.ntp-1];
	  Ssynth = toeplitz(acfsynth);
	  Fsynth = chol(Ssynth)';
	  ynoise = Fsynth*ynoise;
	end % AR1
      end % Synth Noise
      if(SynthSignalAmp ~= 0)
	Xrun = X(indrun,:);
	ysignal = Xrun*ones(nX,nvox); % betasynth = 1
      end
      yrun = ynoise + ysignal;
    end
    if(isempty(yrun))
      fprintf('ERROR: loading %s\n',funcfspec);
      return;
    end
    Brun = B0(:,indrun);
    betamat0 = betamat0 + Brun*yrun;

    clear yrun;
    %pack; % not good with matlab 7.4
  end
  
  % Compute baseline
  betamn0 = mean(betamat0(ind0,:),1);
  % baseline0 = mri;
  % baseline0.vol = fast_mat2vol(betamn0,mri.volsize);
  % Compute Rescale Factor
  if(~isempty(flac0.inorm))
    gmean = mean(betamn0(indmask));
    RescaleFactor = flac0.inorm/gmean;
    fprintf('Global In-Mask Mean = %g\n',gmean);
    fprintf('Rescale Target = %g\n',flac0.inorm);
  else
    RescaleFactor = 1;
  end
  if(DoSynth) RescaleFactor = 1; end
  fprintf('RescaleFactor = %g\n',RescaleFactor);

  betamn0  = RescaleFactor*betamn0;
  betamat0 = RescaleFactor*betamat0;
  
  % Second pass thru the data to compute residual
  fprintf('OLS Residual Pass \n');
  tic;
  rsse = 0;
  rho1 = mri; 
  rho2 = mri; % not really rho2
  for nthrun = nthrunlist
    fprintf('  run %d    t=%4.1f\n',nthrun,toc);
    flac = flac0;
    flac.nthrun = nthrun;
    flac = flac_customize(flac);
    indrun = find(tpindrun == nthrun);
    if(~DoSynth)
      yrun = MRIread(flac.funcfspec);
      if(UseFloat) yrun.vol = single(yrun.vol); end
      yrun = fast_vol2mat(yrun);
    else
      randn('state',yrun_randn(:,nthrun))
      ynoise  = 0;
      ysignal = 0;
      if(SynthNoiseAmp > 0)
	ynoise = randn(flac.ntp,nvox);
	if(SynthNoiseAR1 ~= 0)
	  acfsynth = SynthNoiseAR1.^[0:flac.ntp-1];
	  Ssynth = toeplitz(acfsynth);
	  Fsynth = chol(Ssynth)';
	  ynoise = Fsynth*ynoise;
	end % AR1
      end % Synth Noise
      if(SynthSignalAmp ~= 0)
	Xrun = X(indrun,:);
	ysignal = Xrun*ones(nX,nvox); % betasynth = 1
      end
      yrun = ynoise + ysignal;
    end
    yrun = RescaleFactor*yrun;
    Xrun = X(indrun,:);
    yhatrun = Xrun*betamat0;
    rrun = yrun - yhatrun;
    rsserun = sum(rrun.^2);
    indz = find(rsserun == 0); % keep zeros from screwing stuff up
    rsserun(indz) = max(rsserun);
    rsse = rsse + rsserun;
    rho1run = sum(rrun(1:end-1,:).*rrun(2:end,:))./rsserun;
    rho1.vol(:,:,:,nthrun) = fast_mat2vol(rho1run,rho1.volsize); %bug was here
    %rho2run = sum(rrun(1:end-2,:).*rrun(3:end,:))./rsserun;
    %rho2.vol(:,:,:,end+1) = fast_mat2vol(rho2run,rho2.volsize);
    if(flac0.acfbins == 0)
      %fprintf('WARNING: unwhitened residuals are not intensity norm\n');
      fname = sprintf('%s/res-%03d.%s',outresdir,nthrun,ext);
      rrunmri = mri;
      rrunmri.vol = fast_mat2vol(rrun,mri.volsize);
      MRIwrite(rrunmri,fname);
    end
    clear yrun rrun;
    %pack;
  end
  % Residual variance
  rvarmat0 = rsse/DOF;
  
  rho1mn = mri;
  rho1mn.vol = mean(rho1.vol,4);

  % Save input mask
  fname = sprintf('%s/mask.%s',outanadir,ext);
  MRIwrite(mask,fname);
  
  % Apply bias correction
  nrho1mn = mri;
  nrho1mn.vol = rfm.M(1) + rfm.M(2)*rho1mn.vol;

  % Find places where the correction pushes AR1 > 0.90
  % Probably means there is something wrong with the scan
  ind = find(abs(nrho1mn.vol) > 0.90);
  fprintf('Found %d voxels with corrected AR1 > 0.90\n',length(ind));
  if(length(ind) > 0)
    % This is a bit of a hack to "fix" them - just rescale to
    % within +/-1
    tmp = nrho1mn.vol(ind);
    nrho1mn.vol(ind) = (.9 + .1*abs(tmp)/max(abs(tmp))).*sign(tmp);
  end
  
  % Save AR1 maps
  fname = sprintf('%s/rho1.%s',outanadir,ext);
  MRIwrite(rho1,fname);
  fname = sprintf('%s/rho1mn.%s',outanadir,ext);
  MRIwrite(rho1mn,fname);
  fname = sprintf('%s/nrho1mn.%s',outanadir,ext);
  MRIwrite(nrho1mn,fname);

  clear rho1 rho1mn;
  % Save AR2 maps
  %fname = sprintf('%s/rho2.%s',outanadir,ext);
  %MRIwrite(rho2,fname);
  %rho2mn = mri;
  %rho2mn.vol = mean(rho2.vol,4);
  %fname = sprintf('%s/rho2mn.%s',outanadir,ext);
  %MRIwrite(rho2mn,fname);
  %nrho = rho2mn.vol./rho1mn.vol;
  %nalpha = rho1mn.vol./nrho;
  %indtmp = find(nrho > 1);
  %nrho(indtmp) = 0;
  %nalpha(indtmp) = 0;
  
  % ---------------------------------------------------
  % Segment based on autocorrelation AR1
  acfseg = [];
  acfseg.vol = [];
  acfsegmn = [];
  nrho1segmn = [];
  if(flac0.acfbins > 0)
    fprintf('Whitening\n');

    acfseg = mri;
    acfseg.vol = zeros(acfseg.volsize);
    if(0)
      fprintf('WARNING: using untested whitening\n');
      a = betamn0(indmask);
      a = a/std(a);
      b = rvarmat0(indmask);
      b = b/std(b);
      c = nrho1mn.vol(indmask)';
      ykm = [a; b; c];
      kmeans0 = rand(3,flac0.acfbins);
      [kmeans kmap] = fast_kmeans(ykm,flac0.acfbins,kmeans0);
      acfseg.vol(indmask) = kmap;
    else
      [edge bincenter binmap] = fast_histeq(nrho1mn.vol(indmask), flac0.acfbins);
      %[edge bincenter binmap] = fast_histeq(betamn0(indmask), flac0.acfbins);
      acfseg.vol(indmask) = binmap;
    end
    fname = sprintf('%s/acfseg.%s',outanadir,ext);
    MRIwrite(acfseg,fname);
    clear rvarmat0 betamat0;
    
    % Compute average ar1 in each seg and corresponding acf
    fprintf('Computing whitening matrices\n');
    tic;
    clear rho1segmn nrho2segmn nalphasegmn acfsegmn S Sinv W;
    S    = zeros(ntptot,ntptot,flac0.acfbins);
    Sinv = zeros(ntptot,ntptot,flac0.acfbins);
    W    = zeros(ntptot,ntptot,flac0.acfbins);
    fname = sprintf('%s/acfsegLUT.txt',outanadir);
    fp = fopen(fname,'w');
    for nthseg = 1:flac0.acfbins
      indseg = find(acfseg.vol==nthseg);
      nsegvox = length(indseg);
      if(nsegvox == 0) continue; end
      nrho1segmn(nthseg) = mean(nrho1mn.vol(indseg));
      fprintf(fp,'%2d \t AR1(%0.2f) \t %3d %3d %3d \t %7.4f\n',...
	      nthseg,nrho1segmn(nthseg),...
	      round(255*rand),round(255*rand),round(255*rand),...
	      nrho1segmn(nthseg));
      %nrho1segmn(nthseg) = 0; % No whitening
      acfsegmn(:,nthseg) = nrho1segmn(nthseg).^(nn-1);
      fprintf('  seg  %2d  %5d  nrho1 = %5.3f (t=%4.1f)\n',....
	      nthseg,nsegvox,nrho1segmn(nthseg),toc);
      for nthrun = nthrunlist
	indrun = find(tpindrun == nthrun);
	nnrun = 1:runflac(nthrun).flac.ntp;
	acfsegrun = nrho1segmn(nthseg).^(nnrun-1);
	Srun = toeplitz(acfsegrun);
	Sruninv = inv(Srun);
	Wrun = inv(chol(Sruninv)');
	S(indrun,indrun,nthseg) = Srun;
	Sinv(indrun,indrun,nthseg) = Sruninv;
	W(indrun,indrun,nthseg) = Wrun;
      end
    end
    fclose(fp);

    % First pass thru the data to compute beta
    fprintf('GLS Beta Pass \n');
    tic;
    betamat = zeros(nX,nvox);
    for nthrun = nthrunlist
      fprintf('  run %d    t=%4.1f\n',nthrun,toc);
      flac = flac0;
      flac.nthrun = nthrun;
      flac = flac_customize(flac);
      indrun = find(tpindrun == nthrun);
      if(~DoSynth)
	yrun = MRIread(flac.funcfspec);
	if(UseFloat) yrun.vol = single(yrun.vol); end
	yrun = fast_vol2mat(yrun);
      else
	randn('state',yrun_randn(:,nthrun))
	ynoise  = 0;
	ysignal = 0;
	if(SynthNoiseAmp > 0)
	  ynoise = randn(flac.ntp,nvox);
	  if(SynthNoiseAR1 ~= 0)
	    acfsynth = SynthNoiseAR1.^[0:flac.ntp-1];
	    Ssynth = toeplitz(acfsynth);
	    Fsynth = chol(Ssynth)';
	    ynoise = Fsynth*ynoise;
	  end % AR1
	end % Synth Noise
	if(SynthSignalAmp ~= 0)
	  Xrun = X(indrun,:);
	  ysignal = Xrun*ones(nX,nvox); % betasynth = 1
	end
	yrun = ynoise + ysignal;
      end
      yrun = RescaleFactor*yrun;  
      for nthseg = 0:  flac0.acfbins
	%fprintf('     seg  %d    %g    ---------\n',nthseg,toc);
	indseg = find(acfseg.vol==nthseg);
	if(nthseg == 0)  B = B0;
	else      B = inv(X'*Sinv(:,:,nthseg)*X)*X'*Sinv(:,:,nthseg);
	end
	Brun = B(:,indrun);
	betamat(:,indseg) = betamat(:,indseg) + Brun*yrun(:,indseg);
      end
      clear yrun;
      %pack;
    end
    
    % Second pass thru the data to compute beta
    fprintf('GLS Residual Pass \n');
    err = mkdirp(outresdir);
    if(err) return; end
    tic;
    rsse = 0;
    for nthrun = nthrunlist
      fprintf('  run %d    t=%4.1f\n',nthrun,toc);
      flac = flac0;
      flac.nthrun = nthrun;
      flac = flac_customize(flac);
      indrun = find(tpindrun == nthrun);
      if(~DoSynth)
	yrun = MRIread(flac.funcfspec);
	yrun = fast_vol2mat(yrun);
      else
	randn('state',yrun_randn(:,nthrun))
	ynoise  = 0;
	ysignal = 0;
	if(SynthNoiseAmp > 0)
	  ynoise = randn(flac.ntp,nvox);
	  if(SynthNoiseAR1 ~= 0)
	    acfsynth = SynthNoiseAR1.^[0:flac.ntp-1];
	    Ssynth = toeplitz(acfsynth);
	    Fsynth = chol(Ssynth)';
	    ynoise = Fsynth*ynoise;
	  end % AR1
	end % Synth Noise
	if(SynthSignalAmp ~= 0)
	  Xrun = X(indrun,:);
	  ysignal = Xrun*ones(nX,nvox); % betasynth = 1
	end
	yrun = ynoise + ysignal;
      end
      yrun = RescaleFactor*yrun;
      Xrun = X(indrun,:);
      yhatrun = Xrun*betamat;
      rrun = yrun - yhatrun;
      for nthseg = 1:flac0.acfbins % ok to skip 0
	indseg = find(acfseg.vol==nthseg);
	acfrun = nrho1segmn(nthseg).^([0:flac.ntp-1]');
	Wseg = inv(chol(toeplitz(acfrun))');
	rrun(:,indseg) = Wseg*rrun(:,indseg);
      end
      rsserun = sum(rrun.^2);
      rsse = rsse + rsserun;
      
      clear yrun;
      %pack;

      fname = sprintf('%s/res-%03d.%s',outresdir,nthrun,ext);
      rrunmri = mri;
      rrunmri.vol = fast_mat2vol(rrun,mri.volsize);
      MRIwrite(rrunmri,fname);
      
      clear rrun rrunmri;
      %pack;
    end % run list
    rvarmat = rsse/DOF;
  else
    fprintf('Not Whitening\n');
    rvarmat = rvarmat0;
    betamat = betamat0;
    clear rvarmat0 betamat0;
  end % acfbins > 0

  save(xfile,'X','DOF','flac0','runflac','RescaleFactor',...
       'rfm','acfseg','nrho1segmn','acfsegmn',...
       'DoSynth','SynthSeed','UseFloat','yrun_randn');
  
  % Save baseline as both h-offset and meanfunc
  baseline = mri;
  baseline.vol = fast_mat2vol(mean(betamat(ind0,:),1),mri.volsize);
  fname = sprintf('%s/h-offset.%s',outanadir,ext);
  MRIwrite(baseline,fname);
  fname = sprintf('%s/meanfunc.%s',outanadir,ext);
  MRIwrite(baseline,fname);
  baselinemat = fast_vol2mat(baseline.vol);

  indz  = find(baseline.vol==0);
  indnz = find(baseline.vol~=0);
  fprintf('Found %d zero-valued voxels\n',length(indz));

  beta = mri;
  beta.vol = fast_mat2vol(betamat,beta.volsize);
  fname = sprintf('%s/beta.%s',outanadir,ext);
  MRIwrite(beta,fname);

  rvar = mri;
  rvar.vol = fast_mat2vol(rvarmat,rvar.volsize);
  fname = sprintf('%s/rvar.%s',outanadir,ext);
  MRIwrite(rvar,fname);

  rstd = mri;
  rstd.vol = sqrt(rvar.vol);
  fname = sprintf('%s/rstd.%s',outanadir,ext);
  MRIwrite(rstd,fname);
  
  fsnr = mri;
  fsnr.vol = zeros(fsnr.volsize);
  fsnr.vol(indnz) = baseline.vol(indnz)./rstd.vol(indnz);
  fname = sprintf('%s/fsnr.%s',outanadir,ext);
  MRIwrite(fsnr,fname);
  
  fsnr2 = fsnr;
  fsnr2.vol = fsnr.vol.^2;
  fname = sprintf('%s/fsnr2.%s',outanadir,ext);
  MRIwrite(fsnr2,fname);
  
  if(DoSynth)
    bmn  = mean(betamat(1:nTask,indmask),2);
    bstd = std(betamat(1:nTask,indmask),[],2);
    bvar = bstd.^2;
    fprintf('  Beta Mean Std Var\n');
    fprintf('  %7.4f  %7.4f  %7.4f\n',[bmn bstd bvar]');
  end
    
end % DoGLMFit

if(DoContrasts)
  fprintf('Computing contrasts\n');
  
  if(~DoGLMFit)
    fprintf('Loading previous GLM fit\n');
    load(xfile);
  
    fname = sprintf('%s/h-offset',outanadir);
    baseline = MRIread(fname);
    if(isempty(baseline)) return; end
    indz  = find(baseline.vol==0);
    indnz = find(baseline.vol~=0);
    baseline.vol(indz) = 1e9;
    baselinemat = fast_vol2mat(baseline.vol);

    fname = sprintf('%s/beta',outanadir);
    beta = MRIread(fname);
    if(isempty(beta)) return; end
    betamat = fast_vol2mat(beta.vol);
    clear beta.vol;

    fname = sprintf('%s/rvar',outanadir);
    rvar = MRIread(fname);
    if(isempty(rvar)) return; end
    rvarmat = fast_vol2mat(rvar.vol);
    clear rvar.vol;
    
    if(flac0.acfbins == 0) acfseg.vol = []; end
  end

  %---------------------------------------------------------------%
  fprintf('Starting contrasts\n');
  for nthcon = 1:ncontrasts
    C = flacC.con(nthcon).C;
    [J K] = size(C);
    fprintf('%s J=%d -------------\n',flacC.con(nthcon).name,J);
    if(J==1)
      [Fmat dof1 dof2 cesmat cesvarmat] = ...
	  fast_fratiow(betamat,X,rvarmat,C,acfsegmn,acfseg.vol(:));
    else
      [Fmat dof1 dof2 cesmat] = ...
	  fast_fratiow(betamat,X,rvarmat,C,acfsegmn,acfseg.vol(:));
    end
    pmat = FTest(dof1, dof2, Fmat);
    ind = find(pmat == 0); pmat(ind) = 1;
    fsigmat = -log10(pmat);

    if(DoSynth)
      cmn  = mean(cesmat(:,indmask),2);
      cstd = std(cesmat(:,indmask),[],2);
      cvar = cstd.^2;
      fprintf('  CES Mean Std Var (%g)\n',1/flacC.con(nthcon).vrf);
      fprintf('  %7.4f  %7.4f  %7.4f\n',[cmn cstd cvar]');
      if(SynthSignalAmp == 0)
	nover = length(find(pmat(indmask) < .01));
	pover = nover/nmask;
	[noverlow noverhi] = binomialconf(nmask,.01,90);
	fprintf('  Prob(p < .01) = %g\n',pover);
	fprintf('  nover = %d, conf %d %d  ',nover,noverlow,noverhi);
	if(nover > noverlow & nover < noverhi) fprintf('PASS\n');
	else	                             fprintf('FAIL\n');
	end
      end
    end
    
    % Contrast output
    outcondir = sprintf('%s/%s',outanadir,flacC.con(nthcon).name);
    try 
      % Delete condir if it is there
      fileattrib(outcondir);
      rmdir(outcondir,'s');
    catch; 
    end

    err = mkdirp(outcondir);
    if(err) return; end

    ces = mri;
    ces.vol = fast_mat2vol(cesmat,mri.volsize);
    fname = sprintf('%s/ces.%s',outcondir,ext);
    MRIwrite(ces,fname);
    
    tmp = zeros(J,nvox);
    tmp = cesmat./repmat(abs(baselinemat),[J 1]);
    cespct = mri;
    cespct.vol = fast_mat2vol(tmp,cespct.volsize);
    fname = sprintf('%s/cespct.%s',outcondir,ext);
    MRIwrite(cespct,fname);
    
    fsig = mri;
    fsig.vol = fast_mat2vol(fsigmat,mri.volsize);
    fname = sprintf('%s/fsig.%s',outcondir,ext);
    MRIwrite(fsig,fname);

    if(J == 1)
      t = mri;
      t.vol = sqrt(fsig.vol) .* sign(ces.vol);
      fname = sprintf('%s/t.%s',outcondir,ext);
      MRIwrite(t,fname);
      sig = mri;
      sig.vol = fsig.vol .* sign(ces.vol);
      fname = sprintf('%s/sig.%s',outcondir,ext);
      MRIwrite(sig,fname);
      cesvar = mri;
      cesvar.vol = fast_mat2vol(cesvarmat,mri.volsize);
      fname = sprintf('%s/cesvar.%s',outcondir,ext);
      MRIwrite(cesvar,fname);

      cesvarpct = mri;
      cesvarpct.vol = (100.^2)*cesvar.vol./(baseline.vol.^2);
      fname = sprintf('%s/cesvarpct.%s',outcondir,ext);
      MRIwrite(cesvarpct,fname);
    
      cnr = mri;
      cnr.vol = zeros(cnr.volsize);
      cnr.vol(indnz) = ces.vol(indnz) ./ sqrt(rvar.vol(indnz));
      fname = sprintf('%s/cnr.%s',outcondir,ext);
      MRIwrite(cnr,fname);
    
    end

    if(J > 1)
      % Compute CES amplitude as the sqrt of sum of the squares
      if(J < 20) % 20 to prevent it from going crazy
	fprintf('Computing CES Magnitude\n');
	cesmag = mri;
	cesmag.vol = sqrt(sum(ces.vol.^2,4));
	fname = sprintf('%s/cesmag.%s',outcondir,ext);
	MRIwrite(cesmag,fname);
	
	cesmagpct = mri;
	cesmagpct.vol = 100*cesmag.vol./abs(baseline.vol);
	fname = sprintf('%s/cesmagpct.%s',outcondir,ext);
	MRIwrite(cesmagpct,fname);
      end
    
      tsigmatall = [];
      cesvarmatall = [];
      for nthj = 1:J
	Cj = C(nthj,:);
	[Fmat dof1 dof2 cesmat cesvarmat] = ...
	    fast_fratiow(betamat,X,rvarmat,Cj,acfsegmn,acfseg.vol(:));
	pmat = FTest(dof1, dof2, Fmat);
	ind = find(pmat == 0); pmat(ind) = 1;
	tsigmat = -log10(pmat) .* sign(cesmat);
	tsigmatall(nthj,:) = tsigmat;
	cesvarmatall(nthj,:) = cesvarmat;
      end
      tsigall = mri;
      tsigall.vol = fast_mat2vol(tsigmatall,mri.volsize);
      fname = sprintf('%s/sig.%s',outcondir,ext);
      MRIwrite(tsigall,fname);

      [sigmin rmin]  = max(abs(tsigmatall));
      indmin = sub2ind(size(tsigmatall),rmin,1:nvox);
      % log10(J) is bonf cor 
      sigmin = sign(tsigmatall(indmin)).*(sigmin - log10(J)); 
      tminsig = mri;
      tminsig.vol = fast_mat2vol(sigmin,mri.volsize);
      fname = sprintf('%s/minsig.%s',outcondir,ext);
      MRIwrite(tminsig,fname);

      rminmri = mri;
      rminmri.vol = fast_mat2vol(rmin,mri.volsize);
      fname = sprintf('%s/iminsig.%s',outcondir,ext);
      MRIwrite(rminmri,fname);
      
      cesvarall = mri;
      cesvarall.vol = fast_mat2vol(cesvarmatall,mri.volsize);
      fname = sprintf('%s/cesvar.%s',outcondir,ext);
      MRIwrite(cesvarall,fname);

      tmp = zeros(J,nvox);
      tmp = (100.^2)*cesvarmatall./repmat(baselinemat.^2,[J 1]);
      cesvarpct = mri;
      cesvarpct.vol = fast_mat2vol(tmp,cesvarpct.volsize);
      fname = sprintf('%s/cesvarpct.%s',outcondir,ext);
      MRIwrite(cesvarpct,fname);

    end
  
  end
end % DoContrasts

%------------------------------------------------------%
if(~isempty(analysis) & DoGLMFit)

  % Construct selxavg-style h.dat strucutre for backwards compat
  SumXtX = Ctask*X'*X*Ctask';
  NTaskAvgs = nTask;
  eres_std = sqrt(rvarmat);
  evtaskind = flac_evtaskind(flac0);
  evtask1 = flac0.ev(evtaskind(1));
  hd = fmri_hdrdatstruct;
  hd.TR = flac0.TR;
  if(~isempty(evtask1.psdwin)) 
    hd.TER = evtask1.psdwin(3);
    hd.TimeWindow = evtask1.psdwin(2)-evtask1.psdwin(1);
    hd.TPreStim   = -evtask1.psdwin(1);
  else
    hd.TER = flac0.TR;
    hd.TimeWindow = 0;
    hd.TPreStim   = 0;
  end
  hd.Nc = length(evtaskind)+1;
  Nc = hd.Nc;
  hd.Nnnc = hd.Nc - 1;
  hd.Nh = nTask/hd.Nnnc;
  Navgs_per_cond = hd.Nh;
  hd.DOF = dof2;
  hd.Npercond = 0; % N presentations per cond, who cares?
  hd.Nruns = length(nthrunlist);
  hd.Ntp = ntptot;
  hd.Nrows = mri.volsize(1);
  hd.Ncols = mri.volsize(2);
  hd.Nskip = 0; % Who cares?
  tmp = flac_evindex(flac0,'Poly');
  if(~isempty(tmp))
    hd.DTOrder = flac0.ev(tmp).params(1) + 1;
  else
    hd.DTOrder = 1;
  end
  hd.RescaleFactor = RescaleFactor; 
  hd.HanningRadius = 0.0;
  hd.BrainAirSeg = 0;
  hd.NullCondId    = 0;
  hd.SumXtX        = SumXtX;
  hd.nNoiseAC      = 0;
  hd.CondIdMap     = [0:hd.Nc-1];
  hCovMtx          = Ctask*inv(X'*X)*Ctask';
  hd.hCovMtx       = hCovMtx;
  hd.WhitenFlag    = 0;
  hd.runlist       = nthrunlist;
  hd.funcstem      = flac0.funcstem;
  hd.parname       = flac0.parfile;
  hd.GammaFit = 0; %?
  hd.gfDelta  = [];%?
  hd.gfTau    = [];%?
  %if(s.spmhrf > -1) % Hack
  %  hd.GammaFit = s.spmhrf + 1;
  %  hd.gfDelta = -1*ones(1,s.spmhrf + 1);
  %  hd.gfTau   = -1*ones(1,s.spmhrf + 1);
  %end;
  fname = sprintf('%s/h.dat',outanadir);
  fprintf('Saving h.dat to %s\n',fname);
  fmri_svdat3(fname,hd);
  % hd2 = fmri_lddat3('sxa.dat');

  % -------- Convert to selavg format -------------- %
  hhattmp = betamat(1:NTaskAvgs,:);
  hhattmp = [zeros(Navgs_per_cond,nvox); hhattmp]; % Add zero for cond 0
  hhattmp2 = reshape(hhattmp,[Navgs_per_cond Nc nvox]);
  hstd = sqrt( (diag(hCovMtx).*diag(SumXtX)) * eres_std.^2);
  hstdtmp = hstd(1:NTaskAvgs,:); % Remove offset and baseline
  hstdtmp = [repmat(eres_std, [Navgs_per_cond 1]); hstdtmp]; % Add 0 for cond 0
  hstdtmp2 = reshape(hstdtmp,[Navgs_per_cond Nc nvox]);
  
  %--- Merge Averages and StdDevs ---%
  tmp = zeros(Navgs_per_cond,2,Nc,nvox);
  tmp(:,1,:,:) = hhattmp2;
  tmp(:,2,:,:) = hstdtmp2;
  tmp = reshape(tmp,[Navgs_per_cond*2*Nc mri.volsize ]);
  tmp = permute(tmp,[2 3 4 1]);
  hsxa = mri;
  hsxa.vol = tmp;
  fname = sprintf('%s/h.%s',outanadir,ext);
  MRIwrite(hsxa,fname);
end % sxa format

end % outer run loop

if(exist('okfile','var'))  fmri_touch(okfile); end

return;
