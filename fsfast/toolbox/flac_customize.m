function flacnew = flac_customize(flac)
% flacnew = flac_customize(flac)
%
% Gets number of time points for the given run, loads all stimulus
% timings, and loads all nonpar matrices. 
%
% Caller must set: flac.sess and flac.nthrun
%
% See flac_desmtx for how the design matrices are built.
%
% $Id: flac_customize.m,v 1.2 2004/10/17 18:33:53 greve Exp $

flacnew = [];
if(nargin ~= 1)
 fprintf('flacnew = flac_customize(flac)\n');
 return;
end
flacnew = flac;

% Construct path names
fsdpath = sprintf('%s/%s',flac.sess,flac.fsd);
flacnew.runlist = fast_runlist(fsdpath,flac.runlistfile);
if(isempty(flacnew.runlist))
  fprintf('ERROR: cannot find any runs in %s\n',fsdpath);
  flacnew = [];
  return; 
end
  
flacnew.nruns = length(flacnew.runlist);
if(flacnew.nruns < flac.nthrun)
  fprintf(['ERROR: requested nth run %d is greater than the number of' ...
	   ' runs %d\n'],flacnew.nthrun,flacnew.nruns);
  flacnew = [];
  return; 
end
runid = flacnew.runlist(flac.nthrun,:);

runpath = sprintf('%s/%s',fsdpath,runid);
fstem = sprintf('%s/%s',runpath,flac.funcstem);

% Get the number of time points
[nslices nrows ncols ntp] = fmri_bvoldim(fstem);
if(nslices == 0) 
  fprintf('ERROR: attempting to read %s\n',fstem);
  flacnew = [];
  return; 
end
flacnew.ntp = ntp;

nev = length(flac.ev);
for nthev = 1:nev
  ev = flac.ev(nthev);
  
  % HRF Regressors
  if(ev.ishrf)  
    % Just get the stimulus timing
    stfpath = sprintf('%s/%s',runpath,ev.stf);
    st = fast_ldstf(stfpath);
    if(isempty(st)) 
      fprintf('ERROR: reading timing file %s\n',stfpath);
      flacnew = []; return; 
    end
    flacnew.ev(nthev).st = st;
    continue;
  end  
  
  % Non-parametric regressors - load and demean matrix
  if(strcmp(ev.model,'nonpar'))
    nonparpath = sprintf('%s/%s',runpath,ev.nonparname);
    X = fast_ldbslice(nonparpath);
    if(isempty(X))
      fprintf('ERROR: loading %s\n',nonparpath);
      flacnew = [];
      return;
    end
    if(size(X,2)~=1) X = squeeze(X)';
    else             X = squeeze(X);
    end
    if(size(X,1) ~= ntp)
      fprintf('ERROR: nonpar time point mismatch %s\n',nonparpath);
      flacnew = [];
      return;
    end
    if(size(X,2) < ev.params(1))
      fprintf('ERROR: not enough columns %s\n',nonparpath);
      flacnew = [];
      return;
    end
    % Demean
    X = X(:,1:ev.params(1));
    Xmn = mean(X,1);
    X = X - repmat(Xmn,[ntp 1]);
    flacnew.ev(nthev).X = X;
    continue;
  end

end

return;














