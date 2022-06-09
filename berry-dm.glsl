//
/* Panteleymonov Aleksandr Konstantinovich 2015
//
// if i write this string my code will be 0 chars, :) */

#define iterations 15.0
#define depth 0.0125
#define layers 8.0
#define layersblob 20
#define step 1.0
#define far 10000.0

float radius=0.25; // radius of Snowflakes. maximum for this demo 0.25.
float zoom=4.0; // use this to change details. optimal 0.1 - 4.0.

vec3 light=vec3(0.0,0.0,1.0);
vec2 seed=vec2(0.0,0.0);
float iteratorc=iterations;
float powr;
float res;

vec4 NC0=vec4(0.0,157.0,113.0,270.0);
vec4 NC1=vec4(1.0,158.0,114.0,271.0);

lowp vec4 hash4( mediump vec4 n )
{
	return fract(sin(n)*1399763.5453123);
}
lowp float noise2( mediump vec2 x )
{
	vec2 p = floor(x);
	lowp vec2 f = fract(x);
	f = f*f*(3.0-2.0*f);
	float n = p.x + p.y*157.0;
	lowp vec4 h = hash4(vec4(n)+vec4(NC0.xy,NC1.xy));
	lowp vec2 s1 = mix(h.xy,h.zw,f.xx);
	return mix(s1.x,s1.y,f.y);
}

lowp float noise222( mediump vec2 x, mediump vec2 y, mediump vec2 z )
{
	mediump vec4 lx = vec4(x*y.x,x*y.y);
	mediump vec4 p = floor(lx);
	lowp vec4 f = fract(lx);
	f = f*f*(3.0-2.0*f);
	mediump vec2 n = p.xz + p.yw*157.0;
	lowp vec4 h = mix(hash4(n.xxyy+NC0.xyxy),hash4(n.xxyy+NC1.xyxy),f.xxzz);
	return dot(mix(h.xz,h.yw,f.yw),z);
}

lowp float noise3( mediump vec3 x )
{
	mediump vec3 p = floor(x);
	lowp vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	mediump float n = p.x + dot(p.yz,vec2(157.0,113.0));
	lowp vec4 s1 = mix(hash4(vec4(n)+NC0),hash4(vec4(n)+NC1),f.xxxx);
	return mix(mix(s1.x,s1.y,f.y),mix(s1.z,s1.w,f.y),f.z);
}
lowp vec2 noise3_2( mediump vec3 x )
{
	return vec2(noise3(x),noise3(x+100.0));
}

float map(mediump vec2 rad)
{
	float a;
	if (res<0.0015) {
		//a = noise2(rad.xy*20.6)*0.9+noise2(rad.xy*100.6)*0.1;
		a = noise222(rad.xy,vec2(20.6,100.6),vec2(0.9,0.1));
	} else if (res<0.005) {
		//float a1 = mix(noise2(rad.xy*10.6),1.0,l);
		//a = texture(iChannel0,rad*0.3).x;
		a = noise2(rad.xy*20.6);
		//if (a1<a) a=a1;
	} else {
		a = noise2(rad.xy*10.3);
	}
	return (a-0.5);
}

vec3 distObj(vec3 pos,vec3 ray,float r,vec2 seed)
{
	mediump float rq = r*r;
	mediump vec3 dist = ray*far;

	mediump vec3 norm = vec3(0.0,0.0,1.0);
	mediump float invn = 1.0/dot(norm,ray);
	mediump float depthi = depth;
	if (invn<0.0) {
		depthi =- depthi;
	}
	mediump float ds = 2.0*depthi*invn;
	mediump vec3 r1 = ray*(dot(norm,pos)-depthi)*invn-pos;
	mediump vec3 op1 = r1+norm*depthi;
	mediump float len1 = dot(op1,op1);
	mediump vec3 r2 = r1+ray*ds;
	mediump vec3 op2 = r2-norm*depthi;
	mediump float len2 = dot(op2,op2);

	mediump vec3 n = normalize(cross(ray,norm));
	mediump float mind = dot(pos,n);
	mediump vec3 n2 = cross(ray,n);
	mediump float d = dot(n2,pos)/dot(n2,norm);
	mediump float invd = 0.2/depth;

	if ((len1<rq || len2<rq) || (abs(mind)<r && d<=depth && d>=-depth)) {
		mediump vec3 r3 = r2;
		mediump float len = len1;
		if (len>=rq) {
			mediump vec3 n3 = cross(norm,n);
			mediump float a = inversesqrt(rq-mind*mind)*abs(dot(ray,n3));
			mediump vec3 dt = ray/a;
			r1 =- d*norm-mind*n-dt;
			if (len2>=rq) {
				r2 =- d*norm-mind*n+dt;
			}
			ds = dot(r2-r1,ray);
		}
		ds = (abs(ds)+0.1)/(iterations);
		ds = mix(depth,ds,0.2);
		if (ds>0.01) {
			ds=0.01;
		}
		mediump float ir = 0.35/r;
		r *= zoom;
		ray = ray*ds*5.0;
		for (float m=0.0; m<iterations; m+=1.0) {
			if (m>=iteratorc) {
				break;
			}
			mediump float l = length(r1.xy); //inversesqrt(dot(r1.xy,r1.xy));
			lowp vec2 c3 = abs(r1.xy/l);
			if (c3.x>0.5) {
				c3=abs(c3*0.5+vec2(-c3.y,c3.x)*0.86602540);
			}
			mediump float g = l+c3.x*c3.x; //*1.047197551;
			l *= zoom;
			mediump float h = l-r-0.1;
			l = pow(l,powr)+0.1;
			h = max(h,mix(map(c3*l+seed),1.0,abs(r1.z*invd)))+g*ir-0.245; //0.7*0.35=0.245 //*0.911890636
			if ((h<res*20.0) || abs(r1.z)>depth+0.01) {
				break;
			}
			r1 += ray*h;
			ray*=0.99;
		}
		if (abs(r1.z)<depth+0.01) {
			dist=r1+pos;
		}
	}
	return dist;
}

vec3 nray;
vec3 nray1;
vec3 nray2;
float mxc=1.0;

vec4 filterFlake(vec4 color,vec3 pos,vec3 ray,vec3 ray1,vec3 ray2)
{
	vec3 d=distObj(pos,ray,radius,seed);
	vec3 n1=distObj(pos,ray1,radius,seed);
	vec3 n2=distObj(pos,ray2,radius,seed);

	vec3 lq=vec3(dot(d,d),dot(n1,n1),dot(n2,n2));
	if (lq.x<far || lq.y<far || lq.z<far) {
		vec3 n=normalize(cross(n1-d,n2-d));
		if (lq.x<far && lq.y<far && lq.z<far) {
			nray = n;//normalize(nray+n);
			//nray1 = normalize(ray1+n);
			//nray2 = normalize(ray2+n);
		}
		float da = pow(abs(dot(n,light)),3.0);
		vec3 cf = mix(vec3(0.0,0.4,1.0),color.xyz*10.0,abs(dot(n,ray)));
		cf=mix(cf,vec3(2.0),da);
		color.xyz = mix(color.xyz,cf,mxc*mxc*(0.5+abs(dot(n,ray))*0.5));
	}

	return color;
}

void _mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float time = iTime*0.2;//*0.1;
	res = 1.0 / iResolution.y;
	vec2 p = (-iResolution.xy + 2.0*fragCoord.xy) *res;

	vec3 rotate;

	mat3 mr;

	vec3 ray = normalize(vec3(p,2.0));
	vec3 ray1;
	vec3 ray2;
	vec3 pos = vec3(0.0,0.0,1.0);

	fragColor = vec4(0.0,0.0,0.0,0.0);

	nray = vec3(0.0);
	nray1 = vec3(0.0);
	nray2 = vec3(0.0);

	vec4 refcolor=vec4(0.0);
	iteratorc=iterations-layers;

	vec2 addrot = vec2(0.0);
	if (iMouse.z>0.0) {
		addrot=(iMouse.xy-iResolution.xy*0.5)*res;
	}

	float mxcl = 1.0;
	vec3 addpos=vec3(0.0);
	pos.z = 1.0;
	mxc=1.0;
	radius = 0.25;
	float mzd=(zoom-0.1)/layers;
	for (int i=0; i<layersblob; i++) {
		vec2 p2 = p-vec2(0.25)+vec2(0.1*float(i));
		ray = vec3(p2,2.0)-nray*2.0;
		//ray = nray;//*0.6;
		ray1 = normalize(ray+vec3(0.0,res*2.0,0.0));
		ray2 = normalize(ray+vec3(res*2.0,0.0,0.0));
		ray = normalize(ray);
		vec2 sb = ray.xy*length(pos)/dot(normalize(pos),ray)+vec2(0.0,time);
		seed=floor((sb+vec2(0.0,pos.z)))+pos.z;
		vec3 seedn = vec3(seed,pos.z);
		sb = floor(sb);
		if (noise3(seedn)>0.2 && i<int(layers)) {
			powr = noise3(seedn*10.0)*1.9+0.1;
			rotate.xy=sin((0.5-noise3_2(seedn))*time*5.0)*0.3+addrot;
			rotate.z = (0.5-noise3(seedn+vec3(10.0,3.0,1.0)))*time*5.0;
			seedn.z += time*0.5;
			addpos.xy = sb+vec2(0.25,0.25-time)+noise3_2(seedn)*0.5;
			vec3 sins = sin(rotate);
			vec3 coss = cos(rotate);
			mr=mat3(vec3(coss.x,0.0,sins.x),vec3(0.0,1.0,0.0),vec3(-sins.x,0.0,coss.x));
			mr=mat3(vec3(1.0,0.0,0.0),vec3(0.0,coss.y,sins.y),vec3(0.0,-sins.y,coss.y))*mr;
			mr=mat3(vec3(coss.z,sins.z,0.0),vec3(-sins.z,coss.z,0.0),vec3(0.0,0.0,1.0))*mr;

			light = normalize(vec3(1.0,0.0,1.0))*mr;
			//vec4 cc=filterFlake(fragColor,(pos+addpos)*mr,normalize(ray*mr+nray*0.1),normalize(ray1*mr+nray*0.1),normalize(ray2*mr+nray*0.1));
			vec4 cc = filterFlake(fragColor,(pos+addpos)*mr,ray*mr,ray1*mr,ray2*mr);
			//if (i>0 && dot(nray,nray)!=0.0 && dot(nray1,nray1)!=0.0 && dot(nray2,nray2)!=0.0) refcolor=filterFlake(refcolor,(pos+addpos)*mr,nray,nray1,nray2);
			//cc+=refcolor*0.5;
			fragColor=mix(cc,fragColor,min(1.0,fragColor.w));
		}
		seedn = vec3(sb,pos.z)+vec3(0.5,1000.0,300.0);
		if (noise3(seedn*10.0)>0.4) {
			float raf = 0.3+noise3(seedn*100.0);
			addpos.xy = sb+vec2(0.2,0.2-time)+noise3_2(seedn*100.0)*0.6;
			float l = length(ray*dot(ray,pos+addpos)-pos-addpos);
			l = max(0.0,(1.0-l*10.0*raf));
			fragColor.xyzw += vec4(1.0,1.2,3.0,1.0)*pow(l,5.0)*(pow(0.6+raf,2.0)-0.6)*mxcl;
		}
		mxc -= 1.1/layers;
		pos.z += step;
		iteratorc += 2.0;
		mxcl -= 1.1/float(layersblob);
		zoom-= mzd;
	}

	vec3 cr = mix(vec3(0.0),vec3(0.0,0.0,0.4),(-0.55+p.y)*2.0);
	fragColor.xyz += mix((cr.xyz-fragColor.xyz)*0.1,vec3(0.2,0.5,1.0),clamp((-p.y+1.0)*0.5,0.0,1.0));

	fragColor = min( vec4(1.0), fragColor );
	fragColor.a = 1.0;
}


//#define DEBUG
#define HIGHQ
#define BORDERIZE

const int shader5x7[144] = int[144](
	0x00000000, 0x00000000, 0x000000f6, 0x60006000, 0xfe280000, 0x0028fe28, 0x92ff9264, 0xc8c6004c, 
	0x00c62610, 0x046a926c, 0x0000000a, 0x00000060, 0x42241800, 0x42000000, 0x00001824, 0x083e0814, 
	0x10100014, 0x0010107c, 0x06010000, 0x10100000, 0x00101010, 0x00060600, 0x04020000, 0x00201008, 
	0xa2928a7c, 0x4200007c, 0x000002fe, 0x928a8642, 0x82840062, 0x008cd2a2, 0xfe482818, 0xa2e40008, 
	0x009ca2a2, 0x9292523c, 0x8e80000c, 0x00c0a090, 0x9292926c, 0x9260006c, 0x00789492, 0x006c6c00, 
	0x6d000000, 0x0000006e, 0x82442810, 0x28280000, 0x00282828, 0x10284482, 0x8a800000, 0x00006090, 
	0x525a423c, 0x907e0034, 0x007e9090, 0x929292fe, 0x827c006c, 0x00448282, 0x448282fe, 0x92fe0038, 
	0x00828292, 0x809090fe, 0x827c0080, 0x005c9292, 0x101010fe, 0x828200fe, 0x008282fe, 0xfe818102, 
	0x10fe0080, 0x00824428, 0x020202fe, 0x40fe0002, 0x00fe4020, 0x081020fe, 0x827c00fe, 0x007c8282, 
	0x909090fe, 0x827c0060, 0x007a848a, 0x949890fe, 0x92640062, 0x004c9292, 0x80fe8080, 0x02fc0080, 
	0x00fc0202, 0x040204f8, 0x02fc00f8, 0x00fc021c, 0x281028c6, 0x20c000c6, 0x00c0201e, 0xa2928a86, 
	0xfe0000c2, 0x00008282, 0x08102040, 0x82000004, 0x0000fe82, 0x40804020, 0x01010020, 0x00010101, 
	0x20400000, 0x2a040000, 0x001e2a2a, 0x222214fe, 0x221c001c, 0x00042222, 0x1422221c, 0x2a1c00fe, 
	0x00102a2a, 0x40483e08, 0x25180020, 0x003e2525, 0x202010fe, 0x2200001e, 0x000002be, 0x00be0102, 
	0x08fe0000, 0x00002214, 0x02fe8200, 0x203e0000, 0x001e2018, 0x2020103e, 0x221c001e, 0x001c2222, 
	0x2424183f, 0x24180018, 0x003f1824, 0x2020103e, 0x2a120010, 0x0000242a, 0x227e2020, 0x023c0022, 
	0x00023c02, 0x04020438, 0x023c0038, 0x003c0204, 0x14081422, 0x39000022, 0x003e0505, 0x322a2a26, 
	0x6c100000, 0x00008282, 0x00ee0000, 0x82820000, 0x0000106c, 0x08102010, 0x00000010, 0x00000000
);

const int ipow10[12] = int[12](1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 2147483647, 2147483647);
#define calc_ipow10(x) ipow10[x]

#define _SP 0
#define _BANG 1	// !
#define _QUO 2	// "
#define _NUM 3	// #
#define _DOL 4	// $
#define _PER 5	// %
#define _AMP 6	// &
#define _APO 7	// '
#define _LPA 7	// (
#define _RPA 8	// )
#define _AST 9	// *
#define _PLU 10	// +
#define _COM 11	// ,
#define _MIN 12	// -
#define _DOT 13	// .
#define _SLA 14	// /
#define _0 15	// 0
#define _CL 26	// : colon
#define _SCL 27	// ; semicolon
#define _LT 28	// <
#define _EQ 29	// =
#define _GT 30	// >
#define _QUE 31	// ?
#define _AT 32	// @
#define _A 33
#define _B 34
#define _C 35
#define _D 36
#define _E 37
#define _F 38
#define _G 39
#define _H 40
#define _I 41
#define _J 42
#define _K 43
#define _L 44
#define _M 45
#define _N 46
#define _O 47
#define _P 48
#define _Q 49
#define _R 50
#define _S 51
#define _T 52
#define _U 53
#define _V 54
#define _W 55
#define _X 56
#define _Y 57
#define _Z 58
// [\]^_`

#define _a 65
#define _b 66
#define _c 67
#define _d 68
#define _e 69
#define _f 70
#define _g 71
#define _h 72
#define _i 73
#define _j 74
#define _k 75
#define _l 76
#define _m 77
#define _n 78
#define _o 79
#define _p 80
#define _q 81
#define _r 82
#define _s 83
#define _t 84
#define _u 85
#define _v 86
#define _w 87
#define _x 88
#define _y 89
#define _z 90
// {|}~

#define XPOS		15
#define YPOS		3
#define COLUMN		30
#define MAXCOLUMN	60
#ifdef DEBUG
const int data[1 +COLUMN*8 +MAXCOLUMN*2] = int[](
0,
_S, _E, _S, _S, _I, _O, _N, _SP, _CL,        _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_U, _S, _E, _R, _SP, _SP, _SP, _SP, _CL,     _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_P, _A, _S, _S, _W, _O, _R, _D, _CL,         _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_L, _A, _N, _G, _U, _A, _G, _E, _CL,         _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
_M, _E, _S, _S, _A, _G, _E, _SP, _SP,        _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
_S,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60
);
#endif

#ifdef HIGHQ

// Perform a fake "texel" lookup, and return all 4 cells.
vec4 g5x7d(int ch, vec2 uv)
{
	ivec2 cell = ivec2(uv);
	int x = ch * 6 + cell.x;
	ivec2 xres = ivec2(x, x-1);
	// Fixup gross edges.
	if (cell.x >= 6) xres.x = 0; //Special shader5x7 #0 is all zeroes.
	ivec2 cv = ivec2(shader5x7[xres.x/4], shader5x7[xres.y/4]);
	ivec2 movfs = (xres%4)*8;
	cv = (cv>>movfs)&0xff;
	ivec4 value = ivec4(cv>>(cell.yy-1), cv>>(cell.yy+0))&1;

	return vec4(value.yxwz);
}

vec2 fast_inverse_smoothstep(vec2 x)
{
	// Uncomment for blobbier letters
	//return x;
	return 0.5 - sin(asin(1.0-2.0*x)/3.0); //Inigo Quilez trick.
}

vec2 roundstep(vec2 x)
{
	vec2 coss = cos(x*3.14159 + 3.14159);
	vec2 sins = sign(coss);
	coss = abs(coss);
	coss = pow(coss, vec2(1.0));
	coss *= sins;
	return coss / 2.0 + 0.5;
}

vec3 char5x7(int ch, vec2 uv)
{
#ifdef BORDERIZE
	uv *= vec2(7./6., 9./8.);
	uv += vec2(0.0, 0.0);
#else
	uv += vec2(0., -.25);
#endif
	vec4 d = g5x7d(ch, uv);

	vec2 lp = fast_inverse_smoothstep(fract(uv));

	float top =  mix(d.x, d.y, lp.x);
	float bottom = mix(d.z, d.w, lp.x);
	float v = (mix(top, bottom, lp.y));

	// This makes it be a harder edge (But still kinda soft)
	v = (v-.25)*(4.+ 1./length(dFdx(uv) + dFdy(uv)));

	v = clamp(v, 0., 1.);

	vec3 col = mix(vec3(0, 0, 0)*.5, vec3(uv.y+3., uv.y+3., 10.0)/10.0, float(v));
	return col;
}

#else // HIGHQ

vec3 char5x7(int ch, vec2 uv)
{
#ifdef BORDERIZE
	uv *= vec2(7./6., 9./8.);
	if (uv.x < 0. || uv.y < 0. || uv.x > 6. || uv.y > 8.) return vec3(0);
#endif
	ivec2 cell = ivec2(uv);
	int x = ch * 6 + int(cell.x);
	int cv = shader5x7[x/4];
	int movfs = (x%4)*8;
	int value  = ((cv>>(movfs+cell.y))&1);
	int value2 = ((cv>>(movfs+int(uv.y+.5)))&1);
	if (uv.y >= 7.0) value2 = 0;
	vec3 col = mix(vec3(value2, 0, value2)*.5, vec3(cell.y+3, cell.y+3, 10.0)/10.0, float(value));
	return col;
}

#endif

vec3 print5x7int(int num, vec2 uv, int places, int leadzero)
{
	vec2 cuv = uv*vec2(places, 1.);
	vec2 luv = cuv*vec2(6, 8.);
	ivec2 iuv = ivec2(luv);
	int posi = int(iuv.x)/6;
	int marknegat = -1;
	if (num < 0) marknegat = places-int(log(-float(num))/log(10.0))-2;
	num = abs(num);
	int nn = (num/calc_ipow10(places-posi-1));
	if (posi == marknegat) nn = -3;
	else if (nn <= 0 && posi != places-1) nn = leadzero;
	else nn %= 10;
	int ch = nn+48-32;
	return char5x7(ch, fract(cuv)*vec2(6.,8.));
}

// Zero Leading Integer Print
vec3 print5x7intzl(int num, vec2 uv, int places)
{
	vec2 cuv = uv*vec2(places, 1.);
	vec2 luv = cuv*vec2(6, 8.);
	ivec2 iuv = ivec2(luv);
	int posi = int(iuv.x)/6;
	int nn = (num/calc_ipow10(places-posi-1));
	nn %= 10;
	int ch = nn+48-32;
	return char5x7(ch, fract(cuv)*vec2(6.,8.));
}

vec3 print5x7float(float num, vec2 uv, int wholecount, int decimalcount)
{
	vec2 cuv = uv*vec2(wholecount+decimalcount+1, 1.);
	vec2 luv = cuv*vec2(6, 8.);
	ivec2 iuv = ivec2(luv);
	int posi = int(iuv.x)/6;
	int nn = -2;

	int marknegat = -1;
	if (num < 0.0) {
		marknegat = wholecount-2-int(log(-num)/log(10.0));
	}
    
	num = abs(num);
	num +=  pow(.1f,float(decimalcount))*.499;
	int nv = int(num);
    
	if (posi < wholecount) {
		int wholediff = posi - wholecount+1;
		float v = (pow(10.0 , float(wholediff)));
		int ni = int(float(nv) * v);
		if (posi == marknegat) nn = -3;
		else if (ni <= 0 && wholediff != 0) nn = -16; //Blank out.
		else nn = ni%10;
	} else if (posi > wholecount) {
		num -= float(nv);
		nn = int(num * pow(10.0, float(posi-wholecount)));
		nn %= 10;
	}
	int ch = nn+48-32;

	return char5x7(ch, fract(cuv)*vec2(6, 8.));
}

float getData(int pos)
{
//	return texture(iChannel3, vec2((float(pos) + 0.5) / 256., .5/3.)).r;
	int x = pos % 256;
	int y = pos / 256;
	return texture(iChannel3, vec2((float(x) + 0.5) / 256., (float(y) + 0.5)/3.)).r;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
	// Normalized pixel coords only in X - y still free range.)
	vec2 uv = fragCoord/iResolution.xx;
	uv.y += (1.-(iResolution.y/iResolution.x))/2.0;

	// Rotozoom or adjust the uv coordinates.
	uv -= vec2(.5, .5);
#ifdef ROTOZOOM
	uv.xy*=sin(iTime)*1.+1.;
	float theta = iTime*.1;
	uv = vec2(uv.x*cos(theta)-uv.y*sin(theta),
		  uv.x*sin(theta)+uv.y*cos(theta));
#endif
	uv += vec2(.5, .3);

	// Make our "screen size" 
	uv.y *= 40.0;	// height: 40
	uv.x *= 5.0;	// width : 60 (5*12)

	// Find our line and column.
	vec3 col = vec3(0.0); // black
	int line = 20-int(uv.y);
	int column = int(uv.x*12.);

	if (line>=YPOS && line<=YPOS+6 && column>=XPOS && column<XPOS+COLUMN) {
		// menu
		line -= YPOS;
		column -= XPOS;
#ifdef DEBUG
		int sel = data[0];
		int char = data[1 +line*COLUMN +column];
#else
		int sel = int(getData(0)*255.);
		int char = int(getData(1 +line*COLUMN +column)*255.)-32;
#endif
		col = char5x7(char, mod(uv*vec2(6*12, 8), vec2(6, 8)));

		if (sel*2 == line) col = 1.0 - col; // selected
	} else if (line>=YPOS+9 && line<=YPOS+9 && column>=XPOS && column<XPOS+COLUMN) {
		// message
		column -= XPOS;
#ifdef DEBUG
		int char = data[1 +7*COLUMN +column];
#else
		int char = int(getData(1 +7*COLUMN +column)*255.)-32;
#endif
		col = char5x7(char, mod(uv*vec2(6*12, 8), vec2(6, 8))) +vec3(0.2, 0., 0.);
	} else if (line>=19 && line<20 && column>=0 && column<MAXCOLUMN) {
		// statusbar
		line -= 19;

		uv.x *= 10.0/5.0; // 1/2 font size
		int column = int(uv.x*12.);
#ifdef DEBUG
		col = char5x7(data[1 +COLUMN*8 +MAXCOLUMN*line +column], mod(uv*vec2(6*12, 8), vec2(6, 8)));
#else
		int char = int(getData(1 +COLUMN*8 +MAXCOLUMN*2*line +column)*255.);
		if (char>31 && char<176) char -= 32;
		else char = _SP;
		col = char5x7(char, mod(uv*vec2(6*12, 8), vec2(6, 8)));
#endif
	}

	// Output to screen
//	fragColor = vec4(col, 1.0);

	// Output to screen
	_mainImage(fragColor, fragCoord);
	fragColor += vec4(col, 1.0);
}
