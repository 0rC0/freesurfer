function [vol, M, dcminfo] = load_dicom_series(seriesno,dcmdir,dcmfile)
% [vol, M, dcminfo] = load_dicom_series(seriesno,<dcmdir>,<dcmfile>)
%
% Reads in a dicom series given:
%  1. The series number and directory, or
%  2. A dicom file from the desired series
%
% If the series number is given but no dcmdir is given, then the
% current directory is assumed. All files in the dcmdir are examined
% and the dicom files for the given series are then loaded.
%
% If a dicom file is given, then seriesno and dcmdir are determined
% from the file and file name.
%
% Bugs: will not load multiple frames or mosaics properly.
%
% $Id: load_dicom_series.m,v 1.1 2003/01/23 21:20:38 greve Exp $

if(nargin < 1 | nargin > 3)
  fprintf('[vol, M, dcminfo] = load_dicom_series(seriesno,<dcmdir>,<dcmfile>)\n');
  return;
end

if(nargin == 1) dcmdir = '.'; end
if(nargin == 3) 
  [isdcm dcminfo] = isdicomfile(dcmfile);
  if(~isdcm)
    fprintf('ERROR: %s is not a dicomfile \n',dcmfile);
    return;
  end
  if(~isfield(dcminfo,'SeriesNumber'))
    fprintf('ERROR: %s does not have a series number \n',dcmfile);
    return;
  end
  seriesno = dcminfo.SeriesNumber;
  dcmdir = getdcmdir(dcmfile);
end


% Get a list of files in the directory %
flist = dir(dcmdir);
nfiles = length(flist);
fprintf('INFO: Found %d files in %s\n',nfiles,dcmdir);
if(nfiles == 0)
  fprintf('ERROR: no files in %s\n',dcmdir);
  return;
end

% Determine which ones belong to series %
seriesflist = [];
nth = 1;
fprintf('INFO: searching files for dicom, series %d\n',seriesno);
tic;
for n = 1:nfiles
  if(n==1 | rem(n,20)==0) fprintf('n = %4d, t = %g\n',n,toc); end
  pathname = sprintf('%s/%s',dcmdir,flist(n).name);
  [isdcm dcminfo] = isdicomfile(pathname);
  if(isdcm)
    if(isfield(dcminfo,'SeriesNumber'))
      if(dcminfo.SeriesNumber == seriesno)
        seriesflist = strvcat(seriesflist,pathname);
        dcminfolist(nth) = dcminfo;
        dcminfo0 = dcminfo;
        nth = nth+1;
      end
    end
  end
end
dcminfo = dcminfo0;

fprintf('INFO: search time %g sec\n',toc);
nfilesseries = size(seriesflist,1);
fprintf('INFO: Found %d files in series %d\n',nfiles,seriesno);
if(nfilesseries == 0)
  fprintf('ERROR: no files in series\n');
  return;
end

% Load the volume %
[vol M] = load_dicom_fl(seriesflist);

return;


%---------------------------------------------------%
function dcmdir = getdcmdir(dcmfile)

ind = findstr(dcmfile,'/');
if(~isempty(ind))
  if(max(ind)~=1)
    dcmdir = dcmfile(1:max(ind)-1);
  else
  dcmdir = '/';
  end
else
  dcmdir = '.';
end

return;
