#include <opencv/highgui.h>
#include <opencv/cv.h>
#include <iostream>
#include <time.h>
#include <windows.h>
using namespace std;
using namespace cv;
CvCapture *cap=cvCreateCameraCapture(0);
IplImage *img;
IplImage *gray=cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 1);
IplImage *imga=cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 1);
IplImage *imgb=cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 1);
IplImage *imgc=cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 1);
IplImage *imgnow,*imgtmp;
double Raverage,R0=11.8,Rtmp=0,Rcnt=0;
double mindist=34,dicedist=54,maxdist=76;
int type=0,sg=2,th1=200,th2=30,bw=30;

void Do(int);

bool inMinD(int n){ //center point to corner point
    if(n>(mindist-6)*(mindist-6)&&n<mindist*mindist) return true;
    else return false;
}
bool inDiceD(int n){ //dice edge point size
    if(n<dicedist*dicedist) return true;
    else return false;
}
bool inMaxD(int n){ //diagonal corner
    if(n>(maxdist-20)*(maxdist-20)&&n<maxdist*maxdist) return true;
    else return false;
}
bool inMaxDA(int n){ //all of diagonal corner
    if(n<maxdist*maxdist) return true;
    else return false;
}

int getABS(int a,int b){
    if(a>b) return a-b;
    else return b-a;
}

int getCE(CvSeq *c,int i,int k){ //get circle element
    float *p =(float*)cvGetSeqElem(c,i);
    return cvRound(p[k]);
}

int getDS(CvSeq *c,int s,int t){ //get distance by 2 point
    if(s>=c->total||t>=c->total) return -1;
    else{
        int x1=getCE(c,s,0);
        int y1=getCE(c,s,1);
        int x2=getCE(c,t,0);
        int y2=getCE(c,t,1);
        return ((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2));
}}

int getDCS(CvSeq *c,int s,int xc,int yc){ //get distance by point to circle center
    if(s>=c->total) return -1;
    else{
        int x=getCE(c,s,0);
        int y=getCE(c,s,1);
        return ((x-xc)*(x-xc))+((y-yc)*(y-yc));
}}

bool inDice(int x,int y,int x1,int y1,int x2,int y2){ //use point1,2 judge point3
    int xx1=x1-x,yy1=y1-y,xx2=x2-x,yy2=y2-y;
    if(yy2==0||yy1==0){
        if(inMaxDA((xx2*xx2)+(yy2*yy2)))
            return true;
        else
            return false;
    }
    else{
        int m1=xx1/yy1,m2=xx2/yy2;
        if(yy1>0){
            if((m2>m1&&m2>0)||(m2*m1<-1&&m2<0)&&inMaxDA((xx2*xx2)+(yy2*yy2))) return true;
            else return false;
        }
        else{
            if((m2<m1&&m2<0)||(m2*m1>-1&&m2>0)&&inMaxDA((xx2*xx2)+(yy2*yy2))) return true;
            else return false;
}}}

bool isOne(CvSeq *c,int a,bool *jdg){ //judge the dice is or not one point
    bool one=true;
    int cnt=0;
    for(int i=0;i<c->total;i++){
        if(jdg[i]&&i!=a&&one){
            if(inMinD(getDS(c,a,i))||inDiceD(getDS(c,a,i))) one=false;
            else if(inMaxD(getDS(c,a,i))) cnt++;
    }}
    if(cnt==1) one=false;
    return one;
}

int getCenter(CvSeq *c,int st,int xy,bool *jdg){ //use first point in dice find the center of dice
    int ct=c->total,px=getCE(c,st,0),py=getCE(c,st,1);
    int cnt=1,sx=px,sy=py;

    bool jdg2[ct];
    for(int i=st;i<ct;i++){
        if(jdg[i]&&inMaxD(getDS(c,st,i))) jdg2[i]=true;
        else jdg2[i]=false;
    }

    int st2=st+1;
    for(;st2<ct;st2++)
        if(jdg2[st2]) break;

    int px2=getCE(c,st2,0),py2=getCE(c,st2,1);
    cnt++; sx+=px2; sy+=py2;

    for(int i=st2+1;i<ct;i++){
        if(jdg2[i]&&inDice(px,py,px2,py2,getCE(c,i,0),getCE(c,i,1))){
            cnt++;
            sx+=getCE(c,i,0);
            sy+=getCE(c,i,1);
    }}
    //cout<<"cnt: "<<cnt<<endl;
    if(xy==0) return sx/cnt;
    else if(xy==1) return sy/cnt;
    else return 0;
}

int getDicePoint(CvSeq *c,IplImage *src){ //get point of every dice
    if(c->total==0) return 0;
    int D[3]={0,0,0};
    int st=0,ct=c->total;
    bool hD[ct];
    int x[ct],y[ct];
    CvScalar s;

    for(int i=0;i<ct;i++){
        x[i]=getCE(c,i,0);
        y[i]=getCE(c,i,1);
        s=cvGet2D(src,y[i],x[i]);
        if(s.val[0]==255) hD[i]=true;
        else hD[i]=false;
    }

    for(int i=0;i<ct;i++){ //find the start of dice
        if(hD[i]){
            st=i;
            break;
    }}

    int xtmp[3]={0,0,0},ytmp[3]={0,0,0};
    for(int d=0;d<3;d++){
        if(isOne(c,st,hD)){
            cout<<d<<" "<<st<<":"<<getCE(c,st,0)<<","<<getCE(c,st,1)<<"\nisOne\n"<<endl;
            xtmp[d]=getCE(c,st,0);
            ytmp[d]=getCE(c,st,1);
            hD[st]=false;
            D[d]=1;
        }
        else{
            int xc=getCenter(c,st,0,hD),yc=getCenter(c,st,1,hD);
            xtmp[d]=xc;
            ytmp[d]=yc;
            for(int i=st;i<ct;i++){
                if(D[d]<6&&hD[i]){
                    if(getABS(x[i],xc)<dicedist&&getABS(y[i],yc)<dicedist&&inDiceD(getDCS(c,i,xc,yc))){
                        cout<<d<<" "<<i<<":"<<getCE(c,i,0)<<","<<getCE(c,i,1)<<endl;
                        cout<<getDS(c,st,i)<<" "<<getDCS(c,i,xc,yc)<<endl<<endl;
                        hD[i]=false;
                        D[d]++;
        }}}}
        for(int i=0;i<ct;i++){ //find next start
            if(hD[i]){
                st=i;
                break;
        }}
        if(!hD[st]) break;
    }


    for(int i=0;i<ct;i++){ //process other point
        if(hD[i]){
            cout<<i<<":"<<getCE(c,i,0)<<","<<getCE(c,i,1)<<endl;
            int mark=-1,tmp=640000;
            for(int j=0;j<3;j++){
                if(getDCS(c,i,xtmp[j],ytmp[j])<tmp){
                    tmp=getDCS(c,i,xtmp[j],ytmp[j]);
                    mark=j;
            }}
            if(D[mark]<6){
                D[mark]++;
                hD[i]=false;
            }
            else if(D[1]==0){
                D[1]++;
                hD[i]=false;
            }
            else if(D[2]<6){
                D[2]++;
                hD[i]=false;
            }
            else if(D[1]<6){
                D[1]++;
                hD[i]=false;
            }
            else if(D[0]<6){
                D[0]++;
                hD[i]=false;}
        }}
    return (D[0]*100)+(D[1]*10)+D[2];
}

IplImage *BGR(IplImage *src,int k){ //turn img to B,G,R,gray
    cvSplit(src,imga,imgb,imgc,0);
    cvCvtColor(src,gray,CV_BGR2GRAY);
    if(k=='b') return imga;
    else if(k=='g') return imgb;
    else if(k=='r') return imgc;
    else if(k=='t') return gray;
    else return src;
}

IplImage *HSV(IplImage *src,int k){ //turn img to H,S,V
    cvCvtColor(src,src,CV_BGR2HSV);
    cvSplit(src,imga,imgb,imgc,0);
    if(k=='h') return imga;
    else if(k=='s') return imgb;
    else if(k=='v') return imgc;
    else return src;
}

static int cmp_funcx(const void *_a,const void *_b,void *userdata){ //sort circle by x
    CvPoint *a=(CvPoint*)_a;
    CvPoint *b=(CvPoint*)_b;
    if(a->x>b->x) return 1;
    if(a->x<b->x) return -1;
    return 0;
}

static int cmp_funcxr(const void *_a,const void *_b,void *userdata){ //sort circle by x_INV
    CvPoint *a=(CvPoint*)_a;
    CvPoint *b=(CvPoint*)_b;
    if(a->x>b->x) return -1;
    if(a->x<b->x) return 1;
    return 0;
}

static int cmp_funcy(const void *_a,const void *_b,void *userdata){ //sort circle by y
    CvPoint *a=(CvPoint*)_a;
    CvPoint *b=(CvPoint*)_b;
    if(a->y>b->y) return 1;
    if(a->y<b->y) return -1;
    return 0;
}

static int cmp_funcyr(const void *_a,const void *_b,void *userdata){ //sort circle by y_INV
    CvPoint *a=(CvPoint*)_a;
    CvPoint *b=(CvPoint*)_b;
    if(a->y>b->y) return -1;
    if(a->y<b->y) return 1;
    return 0;
}

CvSeq *Circle(IplImage *src,int minRD,int th1,int th2,int minR,int maxR){ //HoughCircle
    CvMemStorage *hstorage=cvCreateMemStorage(0);
    CvSeq* c=cvHoughCircles(src,hstorage,CV_HOUGH_GRADIENT,1.8,minRD,th1,th2,minR,maxR);
    int minx=639,maxx=0,miny=479,maxy=0;
    int sx=0,sy=0,ct=c->total;
    for(int i=0;i<ct;i++){
        if(getCE(c,i,0)<minx) minx=getCE(c,i,0);
        if(getCE(c,i,0)>maxx) maxx=getCE(c,i,0);
        if(getCE(c,i,1)<miny) miny=getCE(c,i,1);
        if(getCE(c,i,1)>maxy) maxy=getCE(c,i,1);
        sx+=getCE(c,i,0);
        sy+=getCE(c,i,1);
    }
    if(ct!=0){
        sx=sx/ct;
        sy=sy/ct;
        if(maxy-miny>maxx-minx){
            cvSeqSort(c,cmp_funcx,0);
            if(maxy-sy>sy-miny) cvSeqSort(c,cmp_funcy,0);
            else cvSeqSort(c,cmp_funcyr,0);
        }
        else{
            cvSeqSort(c,cmp_funcy,0);
            if(maxx-sx>sx-minx) cvSeqSort(c,cmp_funcx,0);
            else cvSeqSort(c,cmp_funcxr,0);
    }}
    cvReleaseMemStorage(&hstorage);
    return c;
}

CvSeq *CircleFilter(IplImage *src,CvSeq *c,int r){ //filter noise point
    CvScalar s;
    for(int i=0;i<c->total;i++){
        s=cvGet2D(src,getCE(c,i,1),getCE(c,i,0));
        //cout<<s.val[0]<<endl;
        if(r<100){
            if(s.val[0]>r) cvSeqRemove(c,i);
        }
        else{
            if(s.val[0]<r) cvSeqRemove(c,i);
    }}
    return c;
}

void CircleTrackChange(int){
    IplImage *src=cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 1);
    switch(type){
    case 1:
        img=cvQueryFrame(cap);
        cvCopy(BGR(img,'g'),src);
        cvSmooth(src,src,CV_GAUSSIAN,sg*2+1,0);
        cvNot(src,src);
        break;
    case 2:
        img=cvQueryFrame(cap);
        cvCopy(HSV(img,'h'),src);
        cvSmooth(src,src,CV_GAUSSIAN,sg*2+1,0);
        break;
    case 3:
        img=cvQueryFrame(cap);
        cvCopy(HSV(img,'s'),src);
        cvSmooth(src,src,CV_GAUSSIAN,sg*2+1,0);
        break;
    default:
        img=cvQueryFrame(cap);
        cvCopy(BGR(img,'t'),src);
        cvSmooth(src,src,CV_GAUSSIAN,sg*2+1,0);
        cvNot(src,src);
    }
    if(th1>0&&th2>0){
        CvSeq *c=CircleFilter(src,Circle(src,20,th1,th2,7,15),bw);
        for(int i=0;i<c->total;i++){
            cv::Point center(getCE(c,i,0),getCE(c,i,1));
            cvCircle(src,center,1,Scalar(255,255,255),5,8,0);
    }}
    cvShowImage("CircleTrack",src);
    cvZero(src);
    cvReleaseImage(&src);
}

void CircleTrack(){ //circle track mode
    cvNamedWindow("CircleTrack");
    cvCreateTrackbar("type","CircleTrack",&type,3,CircleTrackChange);
    cvCreateTrackbar("Gaussian","CircleTrack",&sg,6,CircleTrackChange);
    cvCreateTrackbar("thresh1","CircleTrack",&th1,600,CircleTrackChange);
    cvCreateTrackbar("thresh2","CircleTrack",&th2,100,CircleTrackChange);
    cvCreateTrackbar("Scalar","CircleTrack",&bw,255,CircleTrackChange);
    while(1){
        CircleTrackChange(0);
        int key=cvWaitKey(1);
        if(key=='c') break;
        if(key==' ')
            cout<<type<<" "<<sg*2+1<<" "<<th1<<" "<<th2<<" "<<bw<<endl;
    }
    cvDestroyWindow("CircleTrack");
}

void draw(CvSeq *c,IplImage *src,int s){ //draw circle
    if(c->total<=0) return;
    for(int i=0;i<c->total;i++){
        cv::Point center(getCE(c,i,0),getCE(c,i,1));
        cvCircle(src,center,1,Scalar(s,s,s),20,8,0);
        //cout<<"R: "<<getCE(c,i,2)<<endl;
        Rtmp+=getCE(c,i,2);
        Rcnt++;
}}

void drawP(CvSeq *c,IplImage *src,int s){ //draw circle center
    for(int i=0;i<c->total;i++){
        cv::Point center(getCE(c,i,0),getCE(c,i,1));
        cvCircle(src,center,1,Scalar(s,s,s),3,8,0);
}}

void myShow(string name,IplImage *src){ //show img
    cvShowImage(name.c_str(),src);
    cvWaitKey(9999);
    cvDestroyWindow(name.c_str());
}

void Do(int key){ //track or test or find dice point
    img=cvQueryFrame(cap);
    if(key=='c') CircleTrack();
    else if(key==' '||key=='t'){
        clock_t start, finish;
        start=clock();
        Raverage=0;
        Rtmp=0;
        Rcnt=0;
        imgtmp=cvCloneImage(BGR(img,'t'));
        cvZero(imgtmp);

        img=cvQueryFrame(cap);
        imgnow=cvCloneImage(BGR(img,'g'));
        cvSmooth(imgnow,imgnow,CV_GAUSSIAN,1,0);
        cvNot(imgnow,imgnow);
        CvSeq *g=CircleFilter(imgnow,Circle(imgnow,20,291,35,5,16),30);
        draw(g,imgtmp,255);
        if(key=='t') myShow("G-Filter",imgtmp);

        img=cvQueryFrame(cap);
        imgnow=cvCloneImage(HSV(img,'h'));
        cvSmooth(imgnow,imgnow,CV_GAUSSIAN,11,0);
        CvSeq *h=CircleFilter(imgnow,Circle(imgnow,20,304,38,5,15),30);
        draw(h,imgtmp,255);
        if(key=='t') myShow("H-Filter",imgtmp);

        img=cvQueryFrame(cap);
        imgnow=cvCloneImage(HSV(img,'s'));
        cvSmooth(imgnow,imgnow,CV_GAUSSIAN,11,0);
        CvSeq *s=CircleFilter(imgnow,Circle(imgnow,20,252,37,5,16),30);
        draw(s,imgtmp,255);
        if(key=='t') myShow("S-Filter",imgtmp);

        img=cvQueryFrame(cap);
        imgnow=cvCloneImage(BGR(img,'g'));
        cvSmooth(imgnow,imgnow,CV_GAUSSIAN,9,0);
        cvNot(imgnow,imgnow);
        CvSeq *g2=CircleFilter(imgnow,Circle(imgnow,20,200,30,5,16),30);
        draw(g2,imgtmp,255);
        if(key=='t') myShow("G-Filter",imgtmp);

        if(Rcnt>0){ //detect cam distance rate by radius
            Raverage=Rtmp/Rcnt;
            mindist=34*Raverage/R0;
            dicedist=54*Raverage/R0;
            maxdist=76*Raverage/R0;
        }

        CvSeq *c=Circle(imgtmp,20,200,10,0,25);
        int D=getDicePoint(c,imgtmp);
        if(key=='t') drawP(c,imgtmp,0);

        cout<<"R_average: "<<Raverage<<endl;
        cout<<mindist<<" "<<dicedist<<" "<<maxdist<<"\n"<<endl;

        int d1=D/100,d2=D/10%10,d3=D%10;
        cout<<d1<<" "<<d2<<" "<<d3<<"   total:"<<d1+d2+d3<<endl;
        finish=clock();
        cout<<finish-start<<" mms\n"<<endl;
        if(key=='t') myShow("Result",imgtmp);
    }
    else cout<<"c:CircleTrack  t:CheckFilter  space:ShowPoint  esc:close"<<endl;
}

int main(){
    while(1){
        img=cvQueryFrame(cap);
        cvShowImage("CamCap",img);
        int key=cvWaitKey(1);
        if(key==27) break;
        else if(key!=-1) Do(key);
    }
    cvReleaseCapture(&cap);
    cvReleaseImage(&gray);
    cvReleaseImage(&imga);
    cvReleaseImage(&imgb);
    cvReleaseImage(&imgc);
    return 0;
}

