#include "s2monitor.h"


S2ScanData::S2ScanData(){
    s2ScanImage = QImage(1,1,QImage::Format_RGB888);
    s2ScanImage.setPixel(0,0,0);
}

void S2ScanData::addNewTile(TileInfo newTileInfo){
    allTiles.append(newTileInfo);
    updateS2ScanImage();
}

void S2ScanData::updateS2ScanImage(){
    float minx = 1000000.0;
    float maxx = -1000000.0;
    float miny = 1000000.0;
    float maxy = -1000000.0;
    long sizex=0;
    long sizey=0;
    QList<LocationSimple> imageLocations;
    for (int i = 0; i<allTiles.length(); i++){
        LocationSimple locationi = allTiles.at(i).getPixelLocation();
        locationi.ev_pc1 = allTiles.at(i).getGalvoLocation().ev_pc1;
        locationi.ev_pc2 = allTiles.at(i).getGalvoLocation().ev_pc2;
        if (locationi.x<minx) minx = locationi.x;
        if (locationi.x+locationi.ev_pc1>maxx) maxx = locationi.x+locationi.ev_pc1;
        if (locationi.y<miny) miny = locationi.y;
        if (locationi.y+ locationi.ev_pc2 >maxy) maxy = locationi.y+locationi.ev_pc1;

        sizex = (long)  (maxx-minx+1.);
        sizey = (long)  (maxy-miny+1.);
    }
    for (int i = 0; i<allTiles.length(); i++){
        LocationSimple locationi = allTiles.at(i).getPixelLocation();
        locationi.x = locationi.x-minx;
        locationi.y = locationi.y-miny;
        imageLocations.append(locationi);
    }
    s2ScanImage  = QImage(sizex,sizey,QImage::Format_RGB888);
    s2ScanImage.fill(0);
    for (int i = 0; i<imageLocations.length(); i++){
        for (long k = imageLocations.at(i).x; k<= (imageLocations.at(i).x+imageLocations.at(i).ev_pc1); k++){
            for (long j = imageLocations.at(i).y; j<= (imageLocations.at(i).y+imageLocations.at(i).ev_pc2); j++){
                QRgb pixelValue =  s2ScanImage.pixel(k,j);
                pixelValue = ((uint) pixelValue ) + (uint) 1;
                s2ScanImage.setPixel(k,j,pixelValue);
            }
        }
    }

    s2ScanImage.save(QString("/local1/dump/testS2Image").append(QString::number(imageLocations.length())).append(".tif"));
}

QImage S2ScanData::getS2ScanImage()const{
    return s2ScanImage;
}

QList<TileInfo> S2ScanData::getAllTileInfo()const{
    return allTiles;
}






S2Monitor::S2Monitor(QWidget *parent) :
    QWidget(parent)
{
}
