float dot2( in vec2 v ) { return dot(v,v); }
float dot2( in vec3 v ) { return dot(v,v); }
float ndot( in vec2 a, in vec2 b ) { return a.x*b.x - a.y*b.y; }

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdRoundBox( vec3 p, vec3 b, float r )
{
  vec3 q = abs(p) - b + r;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

float sdBoxFrame( vec3 p, vec3 b, float e )
 {
        p = abs(p  )-b;
   vec3 q = abs(p+e)-e;
   return min(min(
       length(max(vec3(p.x,q.y,q.z),0.0))+min(max(p.x,max(q.y,q.z)),0.0),
       length(max(vec3(q.x,p.y,q.z),0.0))+min(max(q.x,max(p.y,q.z)),0.0)),
       length(max(vec3(q.x,q.y,p.z),0.0))+min(max(q.x,max(q.y,p.z)),0.0));
 }

 float sdTorus( vec3 p, vec2 t )
 {
   vec2 q = vec2(length(p.xz)-t.x,p.y);
   return length(q)-t.y;
 }

 float sdCappedTorus( vec3 p, vec2 sc, float ra, float rb)
 {
   p.x = abs(p.x);
   float k = (sc.y*p.x>sc.x*p.y) ? dot(p.xy,sc) : length(p.xy);
   return sqrt( dot(p,p) + ra*ra - 2.0*ra*k ) - rb;
 }

 float sdLink( vec3 p, float le, float r1, float r2 )
 {
   vec3 q = vec3( p.x, max(abs(p.y)-le,0.0), p.z );
   return length(vec2(length(q.xy)-r1,q.z)) - r2;
 }

 float sdCylinder( vec3 p, vec3 c )
 {
   return length(p.xz-c.xy)-c.z;
 }

 float sdCone( vec3 p, vec2 c, float h )
 {
   // c is the sin/cos of the angle, h is height
   // Alternatively pass q instead of (c,h),
   // which is the point at the base in 2D
   vec2 q = h*vec2(c.x/c.y,-1.0);

   vec2 w = vec2( length(p.xz), p.y );
   vec2 a = w - q*clamp( dot(w,q)/dot(q,q), 0.0, 1.0 );
   vec2 b = w - q*vec2( clamp( w.x/q.x, 0.0, 1.0 ), 1.0 );
   float k = sign( q.y );
   float d = min(dot( a, a ),dot(b, b));
   float s = max( k*(w.x*q.y-w.y*q.x),k*(w.y-q.y)  );
   return sqrt(d)*sign(s);
 }

 float sdCone( vec3 p, vec2 c, float h )
 {
   float q = length(p.xz);
   return max(dot(c.xy,vec2(q,p.y)),-h-p.y);
 }

 float sdCone( vec3 p, vec2 c )
 {
     // c is the sin/cos of the angle
     vec2 q = vec2( length(p.xz), -p.y );
     float d = length(q-c*max(dot(q,c), 0.0));
     return d * ((q.x*c.y-q.y*c.x<0.0)?-1.0:1.0);
 }

 float sdPlane( vec3 p, vec3 n, float h )
 {
   // n must be normalized
   return dot(p,n) + h;
 }

 float sdTriPrism( vec3 p, vec2 h )
 {
   vec3 q = abs(p);
   return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5);
 }

 float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
 {
   vec3 pa = p - a, ba = b - a;
   float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
   return length( pa - ba*h ) - r;
 }

 float sdVerticalCapsule( vec3 p, float h, float r )
 {
   p.y -= clamp( p.y, 0.0, h );
   return length( p ) - r;
 }

 float sdRoundedCylinder( vec3 p, float ra, float rb, float h )
 {
   vec2 d = vec2( length(p.xz)-2.0*ra+rb, abs(p.y) - h );
   return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rb;
 }

 float sdSolidAngle( vec3 p, vec2 c, float ra )
 {
   // c is the sin/cos of the angle
   vec2 q = vec2( length(p.xz), p.y );
   float l = length(q) - ra;
   float m = length(q - c*clamp(dot(q,c),0.0,ra) );
   return max(l,m*sign(c.y*q.x-c.x*q.y));
 }

 float sdCutSphere( vec3 p, float r, float h )
 {
   float w = sqrt(r*r-h*h);

   vec2 q = vec2( length(p.xz), p.y );
   float s = max( (h-r)*q.x*q.x+w*w*(h+r-2.0*q.y), h*q.x-w*q.y );
   return (s<0.0) ? length(q)-r :
          (q.x<w) ? h - q.y     :
                    length(q-vec2(w,h));
 }

 float sdCutHollowSphere( vec3 p, float r, float h, float t )
 {
   float w = sqrt(r*r-h*h);
   vec2 q = vec2( length(p.xz), p.y );
   return ((h*q.x<w*q.y) ? length(q-vec2(w,h)) :
                           abs(length(q)-r) ) - t;
 }

 float sdEllipsoid( vec3 p, vec3 r )
 {
   float k0 = length(p/r);
   float k1 = length(p/(r*r));
   return k0*(k0-1.0)/k1;
 }


Rhombus - exact   (https://www.shadertoy.com/view/tlVGDc)

float sdRhombus( vec3 p, float la, float lb, float h, float ra )
{
  p = abs(p);
  vec2 b = vec2(la,lb);
  float f = clamp( (ndot(b,b-2.0*p.xz))/dot(b,b), -1.0, 1.0 );
  vec2 q = vec2(length(p.xz-0.5*b*vec2(1.0-f,1.0+f))*sign(p.x*b.y+p.z*b.x-b.x*b.y)-ra, p.y-h);
  return min(max(q.x,q.y),0.0) + length(max(q,0.0));
}

float sdOctahedron( vec3 p, float s)
{
  p = abs(p);
  return (p.x+p.y+p.z-s)*0.57735027;
}

float opUnion( float d1, float d2 )
{
    return min(d1,d2);
}
float opSubtraction( float d1, float d2 )
{
    return max(-d1,d2);
}
float opIntersection( float d1, float d2 )
{
    return max(d1,d2);
}
float opXor( float d1, float d2 )
{
    return max(min(d1,d2),-max(d1,d2));
}

float opSmoothUnion( float d1, float d2, float k )
{
    k *= 4.0;
    float h = max(k-abs(d1-d2),0.0);
    return min(d1, d2) - h*h*0.25/k;
}

float opSmoothSubtraction( float d1, float d2, float k )
{
    return -opSmoothUnion(d1,-d2,k);

    //k *= 4.0;
    // float h = max(k-abs(-d1-d2),0.0);
    // return max(-d1, d2) + h*h*0.25/k;
}

float opSmoothIntersection( float d1, float d2, float k )
{
    return -opSmoothUnion(-d1,-d2,k);

    //k *= 4.0;
    // float h = max(k-abs(d1-d2),0.0);
    // return max(d1, d2) + h*h*0.25/k;
}
