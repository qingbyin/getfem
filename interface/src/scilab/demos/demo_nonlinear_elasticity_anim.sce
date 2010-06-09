lines(0);
stacksize('max');

path = get_absolute_file_path('demo_nonlinear_elasticity_anim.sce');

printf('demo nonlinear_elasticity_anim\n');

// replay all the computations of demo_nonlinear_elasticity.sci

load(path + '/demo_nonlinear_elasticity_U.mat');

nbstep = size(UU,1);
m    = gf_mesh('from string', m_char);
mfu  = gf_mesh_fem('from string',mfu_char,m);
mfdu = gf_mesh_fem('from string',mfdu_char,m);

sl = gf_slice(list('boundary'), m, 16, gf_mesh_get(m,'outer faces'));
P0 = gf_slice_get(sl,'pts');

h = scf();
h.color_map = jetcolormap(255);

for step=1:1:nbstep
  U  = UU(step,:);
  VM = VVM(step,:);
  
  slU  = gf_compute(mfu,U,'interpolate on',sl);
  slVM = gf_compute(mfdu,VM,'interpolate on',sl);
  
  gf_slice_set(sl,'pts', P0+slU);
  
  drawlater;
  clf();
  gf_plot_slice(sl, 'data', slVM, 'mesh_edges','on', 'mesh','on'); 
  drawnow;

  //drawlater;
  //gf_plot(mfdu,VM,'mesh','on', 'cvlst',gf_mesh_get(mfdu,'outer faces'), 'deformation',U,'deformation_mf',mfu,'deformation_scale', 1, 'refine', 16); 
  //drawnow;
//  axis([-3     6     0    20    -2     2]);
//  caxis([0 .15]);
//  view(30+20*w, 23+30*w);  
//  campos([50 -30 80]);
//  camva(8);
//  camup;
//  camlight; 
//  axis off;
  xs2png(h.figure_id, path + sprintf('/torsion%03d.png',step));
end

printf('demo nonlinear_elasticity_anim terminated\n');
