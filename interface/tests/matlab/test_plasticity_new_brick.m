clear all
gf_workspace('clear all');
clc

%%

% We try to compute a plasticity problem with a Von Mises crierion
% For convenience we consider an homogenous Dirichlet condition on the left
% of the domain and an easy computed Neumann Condition on the right

%%



% Initialize used data
L = 100;
H = 20;
lambda = 121150;
mu = 80769;
von_mises_threshold = 7000;
f = [0 -330]';
t = [0 0.9032 1 1.1 1.3 1.5 1.7 1.74 1.7 1.5 1.3 1.1 1 0.9032 0.7 0.5 0.3 0.1 0];

% Create the mesh
m = gfMesh('triangles grid', [0:4:L], [0:2:H]);

% Plotting
gf_plot_mesh(m, 'vertices', 'on', 'convexes', 'on');

% Define used MeshIm
mim=gfMeshIm(m);  set(mim, 'integ',gfInteg('IM_TRIANGLE(6)')); % Gauss methods on triangles

% Define used MeshFem
mf_u=gfMeshFem(m,2); set(mf_u, 'fem',gfFem('FEM_PK(2,1)'));
mf_data=gfMeshFem(m); set(mf_data, 'fem', gfFem('FEM_PK(2,0)'));
mf_sigma=gfMeshFem(m,4); set(mf_sigma, 'fem',gfFem('FEM_PK_DISCONTINUOUS(2,0)'));
mf_err=gfMeshFem(m); set(mf_err, 'fem',gfFem('FEM_PK(2,0)'));
mf_vm = gfMeshFem(m); set(mf_vm, 'fem', gfFem('FEM_PK_DISCONTINUOUS(2,1)'));
% Find the border of the domain
P=get(m, 'pts');
pidleft=find(abs(P(1,:))<1e-6);
pidright=find(abs(P(1,:) - L)<1e-6);
fleft =get(m,'faces from pid',pidleft); 
fright=get(m,'faces from pid',pidright);

% Assign boundary numbers
set(m,'boundary',1,fleft); % for Dirichlet condition
set(m,'boundary',2,fright); % for Neumann condition

% Create the model
md = gfModel('real');

% Declare that u is the unknown of the system on mf_u
% 2 is the number of version of the data stored, for the time integration scheme 
set(md, 'add fem variable', 'u', mf_u, 2);

% Declare that lambda is a data of the system on mf_data
set(md, 'add initialized data', 'lambda', lambda);

% Declare that mu is a data of the system on mf_data
set(md, 'add initialized data', 'mu', mu);

% Declare that von_mises_threshold is a data of the system on mf_data
set(md, 'add initialized data', 'von_mises_threshold', von_mises_threshold);

% Declare that sigma is a data of the system on mf_sigma
% 2 is the number of version of the data stored, for the time integration scheme
set(md, 'add fem data', 'sigma', mf_sigma);

% Add plasticity brick on u
set(md, 'add elastoplasticity brick', mim, 'VM', 'u', 'lambda', 'mu', 'von_mises_threshold', 'sigma');

% Add homogeneous Dirichlet condition to u on the the left hand side of the domain
set(md, 'add Dirichlet condition with multipliers', mim, 'u', mf_u, 1);

% Add a source term to the system
set(md,'add initialized fem data', 'VolumicData', mf_data, get(mf_data, 'eval',{f(1,1);f(2,1)*t(1)}));
set(md, 'add source term brick', mim, 'u', 'VolumicData', 2);

VM=zeros(1,get(mf_vm, 'nbdof'));


nbstep = size(t,2);

dd = get(mf_err, 'basic dof from cvid');

for step=1:nbstep,
    if step > 1
        set(md, 'variable', 'VolumicData', get(mf_data, 'eval',{f(1,1);f(2,1)*t(step)}));
    end
    
    % Solve the system
    get(md, 'solve','lsolver', 'superlu', 'very noisy', 'max_iter', 1000, 'max_res', 1e-6);

    % Retrieve the solution U
    U = get(md, 'variable', 'u', 0);
    
    % Compute new plasticity constraints used to compute 
    % the Von Mises or Tresca stress
    get(md, 'compute plasticity constraints', mim, 'u', 'VM', 'lambda', 'mu', 'von_mises_threshold', 'sigma');
      
    % Compute Von Mises or Tresca stress
    VM = get(md, 'compute plasticity Von Mises or Tresca', 'sigma', mf_vm, 'Von Mises');
    max(abs(VM));
  
    subplot(2,1,1);
    gf_plot(mf_vm,VM,'deformed_mesh','on', 'deformation',U,'deformation_mf',mf_u,'refine', 4, 'deformation_scale',1); 
    colorbar;
    caxis([0 10000]);
    n = t(step);
    title(['Von Mises criterion for t = ', num2str(n)]);
  
    ERR=gf_compute(mf_u,U,'error estimate', mim);
    E=ERR; E(dd)=ERR;
    subplot(2,1,2);
    gf_plot(mf_err, E, 'mesh','on', 'refine', 1); 
    colorbar;
    title('Error estimate');

    pause;

end;







