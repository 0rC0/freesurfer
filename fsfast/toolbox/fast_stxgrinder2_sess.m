% fast_stxgrinder2_sess
% $Id: fast_stxgrinder2_sess.m,v 1.3 2003/08/11 22:48:03 greve Exp $

% These variables must be defined previously
% SessList = splitstring('$SessList');
% fsd      = '$fsd';
% analysis = '$analysis';
% contrasts = splitstring('$contrastlist');
% hemi = splitstring('$hemi');
% spacedir = '$spacedir';
% tTestDOFMax = $tTestDOFMax;
% FTestDOFMax = $FTestDOFMax;
% DoFTest = $DoFTest;
% tTestSave = $tTestSave;
% IsGroup = [$IsGroupList];

tic;
nsess = size(SessList,1);
nhemi = size(hemi,1);
ncontrasts = size(contrasts,1);

fprintf('\n\n');

for nthsess = 1:nsess
  sessdir = deblank(SessList(nthsess,:));
  fprintf('nthsess = %d  time=%g -----------------\n',nthsess,toc);
  fprintf('%s\n',sessdir);

  for nthhemi = 1:nhemi

    hid = deblank(hemi(nthhemi,:));
    if(strcmp(hid,'nohemi'))  
      hemicode = '';
    else                       
      fprintf('hemi = %s   (%g)\n',hid,toc);
      hemicode = sprintf('-%s',hid);
    end

    if(IsGroup(nthsess))
      sessanadir = sprintf('%s/%s/%s/%s-ffx',sessdir,fsd,analysis,spacedir);
    else
      sessanadir = sprintf('%s/%s/%s/%s',sessdir,fsd,analysis,spacedir);
    end
    hstem = sprintf('%s/h%s',sessanadir,hemicode);
    h0stem = sprintf('%s/h%s-offset',sessanadir,hemicode);

    % get the dim
    [nrows ncols nframes fs nslices endian bext] = fmri_bfiledim(hstem);
    if(isempty(nrows))
      fprintf('ERROR: loading %s\n',hstem);
      return;
    end

    mristruct = fast_ldbhdr(hstem);

    % Contrast Loop
    for c = 1:ncontrasts
      contrast = deblank(contrasts(c,:));
      fprintf('  contrast %s  (time=%g)\n',contrast,toc);
      condir = sprintf('%s/%s',sessanadir,contrast);

      cmat = sprintf('%s/%s.mat',analysis,contrast);
      tmp = load(cmat);
      if(isempty(tmp))
        fprintf('ERROR: loading %s\n',cmat);
        return;
      end
      C = tmp.ContrastMtx_0;
      J = size(C,1);

      % Loop over each slice separately %
      fprintf('     slice ');
      for slice = 0:nslices-1
        fprintf('%d ',slice);
	if(rem(slice,21)==20) fprintf('\n           '); end

        % Load beta %
        hAvgFile = sprintf('%s_%03d.bfloat',hstem,slice);
        [beta rvar hd] = fast_ldsxabfile(hAvgFile);
        if(isempty(beta))
          fprintf('ERROR: loading %s\n',hAvgFile);
          return;
        end
        Ch = hd.hCovMtx;

        [nrows ncols nbeta] = size(beta);
        nv = nrows*ncols;
        beta = reshape(beta,[nv nbeta])';
        rvar = reshape(rvar,[nv 1])';
        ind = find(rvar == 0);
        rvar(ind) = 10e10;

        % Load mean offset %
        h0 = fast_ldbslice(h0stem,slice);
        if(isempty(h0))
          fprintf('ERROR: loading %s\n',h0stem);
          return;
        end
        ind = find(h0==0);
        h0(ind) = 10e10;
        h0 = reshape(h0,[nv 1])';

        if(size(C,2) ~= nbeta)
	  fprintf('\n');
	  fprintf('ERROR: size mismatch between analysis %s and contrast %s.\n',...
                  analysis,contrast);
          fprintf('This usually happens when the parameters of an analysis\n');
          fprintf('have been changed without re-creating the contrast.\n');
          fprintf('Try re-running mkcontrast-sess for this contrast.\n');
	  fprintf('\n');
          return;
        end

        % Go through each row of C separately %
        % Dont have to use a loop here, just easier
        ces    = zeros(J,nv);
        cesvar = zeros(J,nv);
        t      = zeros(J,nv);
        p      = zeros(J,nv);
        for k = 1:J
          Crow = C(k,:);
          cesrow = Crow*beta;
          cesvarrow = rvar * (Crow * Ch * Crow');
          trow = cesrow./sqrt(cesvarrow);
          prow = sign(trow).*tTest(hd.DOF,abs(trow),tTestDOFMax);
          ces(k,:)    = cesrow;
          cesvar(k,:) = cesvarrow;
          t(k,:)      = trow;
          p(k,:)      = prow;
        end

        if(tTestSave & ~strcmp(contrast,'omnibus') & ...
	   ~strcmp(contrast,'zomnibus'))
          cesstem = sprintf('%s/ces%s',condir,hemicode);
          tmp = reshape(ces', [nrows ncols J]);
          fast_svbslice(tmp,cesstem,slice,'',mristruct);

          cespctstem = sprintf('%s/cespct%s',condir,hemicode);
          tmp = 100*(ces./repmat(h0,[J 1]));
          tmp = reshape(tmp', [nrows ncols J]);
          fast_svbslice(tmp,cespctstem,slice,'',mristruct);

          cesvarstem = sprintf('%s/cesvar%s',condir,hemicode);
          tmp = reshape(cesvar', [nrows ncols J]);
          fast_svbslice(tmp,cesvarstem,slice,'',mristruct);

          tstem = sprintf('%s/t%s',condir,hemicode);
          tmp = reshape(t', [nrows ncols J]);
          fast_svbslice(tmp,tstem,slice,'',mristruct);

          pstem = sprintf('%s/sig%s',condir,hemicode);
	  tmp = p; indz = find(p==0); tmp(indz) = 1;
          tmp = -sign(tmp) .* log10(abs(tmp));
          tmp = reshape(tmp', [nrows ncols J]);
          fast_svbslice(tmp,pstem,slice,'',mristruct);
        end

        % Handle multiple rows in C %
        if(J > 1)
          % Min sig with bonferroni correction
          [ptmp imin] = min(abs(p));
          ind = sub2ind(size(p),imin,1:nv);
          pmin = J*p(ind);
          
          pminstem = sprintf('%s/minsig%s',condir,hemicode);
	  tmp = pmin; indz = find(pmin==0); tmp(indz) = 1;
          tmp = -sign(tmp) .* log10(abs(tmp));
          tmp = reshape(tmp', [nrows ncols 1]);
          fast_svbslice(tmp,pminstem,slice,'',mristruct);

          iminstem = sprintf('%s/iminsig%s',condir,hemicode);
          tmp = reshape(imin', [nrows ncols 1]);
          fast_svbslice(tmp,iminstem,slice,'bshort',mristruct);
        end % Handle multiple rows in C

        % F-test
        if(DoFTest)
          cescvm = inv(C*Ch*C');
          if(J>1) F = (sum(ces .* (cescvm*ces))./rvar)/J;
          else    F = t.^2;
          end
          Fsig = FTest(J, hd.DOF, F, FTestDOFMax);

          Fstem = sprintf('%s/f%s',condir,hemicode);
          tmp = reshape(F', [nrows ncols 1]);
          fast_svbslice(tmp,Fstem,slice,'',mristruct);

          Fsigstem = sprintf('%s/fsig%s',condir,hemicode);
	  tmp = Fsig; indz = find(Fsig==0); tmp(indz) = 1;
          tmp = -log10(abs(tmp)); % dont adjust sign
          tmp = reshape(tmp', [nrows ncols 1]);
          fast_svbslice(tmp,Fsigstem,slice,'',mristruct);
        end % FTest
      end % slice
      fprintf('\n');

    end % loop over contrasts      

  end % hemi
  fprintf('\n');
  fprintf('\n');
end % sess

fprintf('\n');
fprintf('done %g\n',toc);
fprintf('\n');

