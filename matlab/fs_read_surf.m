function Surf = fs_read_surf(fname)
% Surf = fs_read_surf(fname)
% 
% Reads coordinates and triangles of a Freesurfer's surf (Depends on 
% Freesurfer's read_surf function).
%
% Input
% fname: A Freesurfer's surface file.
%
% Output
% Surf: Surface which represent a coordinate system where the analysis is 
% to be done. It is a structure with Surf.tri = t x 3 matrix of triangle 
% indices, 1-based, t=#triangles and Surf.coord = 3 x nv matrix of 
% coordinates, nv=#vertices.
%
% $Revision: 1.1 $  $Date: 2012/11/15 15:17:51 $
% Original Author: Jorge Luis Bernal Rusiel 
% CVS Revision Info:
%    $Author: vinke $
%    $Date: 2012/11/15 15:17:51 $
%    $Revision: 1.1 $
%
[coord,faces] = read_surf(fname);
Surf.tri = uint32(faces + 1);
Surf.coord = coord';