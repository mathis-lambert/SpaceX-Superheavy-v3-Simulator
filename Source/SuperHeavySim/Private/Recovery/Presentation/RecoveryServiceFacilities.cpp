#include "Recovery/Presentation/RecoverySiteDetailsComponent.h"

void URecoverySiteDetailsComponent::BuildServiceFacilities()
{
    // Authored maintenance yard. Low structures remain outside the mount and
    // catch hardware; all parts share the existing eight instanced batches.
    for(int Bay=0;Bay<3;++Bay)
    {
        const double X=133+Bay*24;
        Box(2,FVector(X,40,.2),FVector(23,26,.4));
        Box(1,FVector(X,47,3.3),FVector(22,12,6.2));
        Box(0,FVector(X,47,6.5),FVector(23,13,.22));
        for(double Side:{-10.7,10.7})Box(0,FVector(X+Side,47,3.3),FVector(.18,12,6.1));
        for(int Rib=0;Rib<30;++Rib)Box(0,FVector(X-10.4+Rib*.72,53.04,3.3),FVector(.055,.12,6));
        // Recessed roller doors with individual slats and protective bollards.
        Box(0,FVector(X,40.9,2.9),FVector(14,.12,5));
        for(int Slat=0;Slat<20;++Slat)Box(1,FVector(X,40.82,.6+Slat*.24),FVector(14,.025,.025));
        for(double Side:{-8.,8.})Pipe(3,FVector(X+Side,38,.15),FVector(X+Side,38,1.25),.18);
        Box(0,FVector(X+6,48,7),FVector(3.2,2.5,.9));
        Pipe(1,FVector(X+6,48,7.45),FVector(X+6,48,7.65),1.65);
        Pipe(0,FVector(X-10.3,52.5,.2),FVector(X-10.3,52.5,6.6),.12);
        // Parking marks and wheel stops use centimetre-thick geometry.
        for(int Space=0;Space<3;++Space)
        {
            const double PX=X-7+Space*7;
            Box(0,FVector(PX,27,.077),FVector(.12,9,.012));
            Box(2,FVector(PX+3.5,30,.17),FVector(2.4,.3,.2));
        }
    }
    // Container workshop, external cable reels and a loading platform.
    for(int Container=0;Container<3;++Container)
    {
        const FVector P(105+Container*15,-53,1.5);
        Box(Container==1?1:0,P,FVector(12,2.5,2.8));
        for(int Rib=0;Rib<29;++Rib)Box(1,P+FVector(-5.8+Rib*.41,-1.27,0),FVector(.035,.045,2.65));
        for(double Y:{-.8,.8})Pipe(0,P+FVector(6.04,Y,-1.1),P+FVector(6.04,Y,1.1),.04);
        Box(2,P-FVector(0,0,1.3),FVector(13,3.5,.4));
    }
    for(int Reel=0;Reel<4;++Reel)
    {
        const FVector P(168+Reel*3,-53,1.2);
        Pipe(1,P-FVector(0,.5,0),P+FVector(0,.5,0),1.8);
        for(double Y:{-.58,.58})Pipe(0,P+FVector(0,Y-.07,0),P+FVector(0,Y+.07,0),2.2);
    }
    // Deliberate service paving around the apron with a graded shoulder, plus
    // drainage grates: different edge widths break the old isolated slab look.
    for(double Y:{-131.,131.})Box(2,FVector(65,Y,-.32),FVector(430,12,.3));
    for(double X:{-154.,284.})Box(2,FVector(X,0,-.32),FVector(8,270,.3));
    for(int Grate=0;Grate<22;++Grate)
    {
        const double X=-139+Grate*19;
        for(double Y:{-123.,123.})
        {
            Box(1,FVector(X,Y,.002),FVector(2,.45,.024));
            for(int Bar=0;Bar<10;++Bar)Box(0,FVector(X-.9+Bar*.2,Y,.023),FVector(.045,.45,.012));
        }
    }
}
