/* Branch_detection_plugin.h
 * This is a test plugin, you can use it as a demo.
 * 2018-5-20 : by Chao Wang
 */
 
#ifndef __BRANCH_DETECTION_PLUGIN_H__
#define __BRANCH_DETECTION_PLUGIN_H__

#include <QtGui>
#include <v3d_interface.h>
using namespace std;

class TestPlugin : public QObject, public V3DPluginInterface2_1
{
	Q_OBJECT
    Q_INTERFACES(V3DPluginInterface2_1)

public:
	float getPluginVersion() const {return 1.1f;}

	QStringList menulist() const;
	void domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent);

	QStringList funclist() const ;
	bool dofunc(const QString &func_name, const V3DPluginArgList &input, V3DPluginArgList &output, V3DPluginCallback2 &callback, QWidget *parent);
};
int use_rayshooting(V3DPluginCallback2 &callback, QWidget *parent);
int branch_detection(V3DPluginCallback2 &callback, QWidget *parent);
int rayshoot_model(V3DPluginCallback2 &callback, QWidget *parent);
int MIPZSlices( V3DPluginCallback2 &callback, QWidget *parent);
bool mip_z_slices(Image4DSimple * subject, Image4DSimple & outImage,
             V3DLONG startnum, V3DLONG increment, V3DLONG endnum);
void printHelp();
int branch_detection_single(V3DPluginCallback2 &callback, QWidget *parent);
int new_branch_detection(V3DPluginCallback2 &callback, QWidget *parent);
int skeletonization(V3DPluginCallback2 &callback, QWidget *parent);
int thin(V3DPluginCallback2 &callback, QWidget *parent);
int display_rayshooting_model_in_xyproject(V3DPluginCallback2 &callback, QWidget *parent);
int display_length_model(V3DPluginCallback2 &callback,QWidget *parent);
int create_groundtruth(V3DPluginCallback2 &callback,QWidget *parent);
int rotate_2D_display_point(V3DPluginCallback2 &callback,QWidget *parent);
int rotate_2D_image(V3DPluginCallback2 &callback,QWidget *parent);
int rotate_3D_image(V3DPluginCallback2 &callback,QWidget *parent);
int rotate_3D_display_point(V3DPluginCallback2 &callback,QWidget *parent);
int rotate_3D_slice_image(V3DPluginCallback2 &callback,QWidget *parent);


#endif

