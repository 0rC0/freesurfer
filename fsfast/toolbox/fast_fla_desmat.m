function X = fast_fla_desmat(flacfg,mthrun,mthruntype)
% X = fast_fla_desmat(flacfg,<mthrun,mthruntype>)
% mthruntype = 0 or [] (all runs), 1 (perrun), 2 (jkrun)

X = [];

if(nargin < 1 | nargin > 3)
  fprintf('X = fast_fla_desmat(flacfg,<mthrun,mthruntype>)\n');
  return;
end
if(nargin == 2)
  fprintf('ERROR: must specify mthruntype with mthrun\n');
  return;
end
if(exist('mthrun') ~= 1) mthrun = []; end
if(isempty(flacfg.sesscfg.runlist))
  fprintf('ERROR: no runs in sesscfg run list\n');
  return;
end
if(isempty(flacfg.fxlist))
  fprintf('ERROR: no fx defined in fla.\n');
  return;
end

nruns = length(flacfg.sesscfg.runlist);
nfx = length(flacfg.fxlist);

% Get the list of run indices to use %
if(~isempty(mthrun))
  if(mthrun > nruns) 
    fprintf('ERROR: mthrun = %d > nruns = %d\n',mthrun,nruns);
    return;
  end
  if(mthruntype == 0) mthrunlist = 1:nruns; end % do all runs
  if(mthruntype == 1) mthrunlist = mthrun; end  % per run 
  if(mthruntype == 2)                           % jk run 
    mthrunlist = 1:nruns;
    ind = find(mthrunlist~=mthrun);
    mthrunlist = mthrunlist(ind);
  end
else
  mthrunlist = 1:nruns; 
end  

% Build the Design Matrix
Xfe = []; % fixed  part
Xre = []; % random part
for nthrun = mthrunlist

  % build each run separately
  flacfg.nthrun = nthrun;
  Xferun = [];
  Xrerun = [];

  for nthfx = 1:nfx
    % build each effect 
    flacfg.nthfx = nthfx;
    fx = fast_fxcfg('getfxcfg',flacfg);
    if(isempty(fx)) return; end

    % Get matrix for this effect
    Xtmp = fast_fxcfg('matrix',flacfg);
    if(isempty(Xtmp)) return; end
    
    % Append it to the proper type
    if(strcmp(fx.fxtype,'fixed'))  Xferun = [Xferun Xtmp];  end
    if(strcmp(fx.fxtype,'random')) Xrerun = [Xrerun Xtmp]; end
  end

  % Accumulated fixed effects
  Xfe = [Xfe; Xferun]; % Should check size here

  % Accumulated random effects
  Za = zeros(size(Xre,1),size(Xrerun,2)); % zero padding
  Zb = zeros(size(Xrerun,1),size(Xre,2)); % zero padding
  Xre = [Xre Za; Zb Xrerun];
  
end

% Finally!
X = [Xfe Xre];


return;