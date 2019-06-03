#version 420
uniform sampler2D tex;
uniform float iTime;
out vec4 fragCol;

// #define MAXDEPTH 3
// #define SAMPLES 1

float polarized;// = false;

vec3 ML(float F) {
    return mat3(1.5,-.2,-.1,-.6,1.1,.2,-.1,.1,1.5)*(.5-.5*cos(2.*F*vec3(5.2,5.7,7.2)));
}

struct Ray
{
  vec3 m_origin;
  vec3 m_direction;
  vec3 m_point;
  vec3 m_color;
  float m_attenuation;
  float m_lag;
};

float sdLine(vec2 p, vec2 a, vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

float sidecond(vec2 p, vec2 a, vec2 b) {
    return dot(p-b, (a-b).yx*vec2(-1.,1.));
}

float sdProfile(vec2 p) {
    vec2 a = vec2(0.0);
    vec2 c = vec2(0.05, 0.4);
    vec2 d = vec2(0.05, 5.0);
    vec2 e = vec2(0.0, 5.0);
    float dist = min(min(sdLine(p,e,a), sdLine(p,c,d)), sdLine(p,a,c));
    if (sidecond(p, a, c) < 0.0
        && sidecond(p, e, a) < 0.0
        && sidecond(p, c, d) < 0.0) dist *= -1.0;
    return dist;
}

float half_ring(vec2 p, float radius, float thickness) {
    return max(abs(length(p)-radius)-thickness,-0.5-p.x);
}

float square(vec2 p, vec2 b)
{
    vec2 d = abs(p)-b;
    return length(max(d,vec2(0))) + min(max(d.x,d.y),0.0);
}

float scene(vec3 p) {
    float ruler = sdProfile(vec2(p.z,-square(p.xy, vec2(0.8,4.0))))-0.05;
    float ruler_hole = min(length(p.xy+vec2(0.0,3.0))-0.3, length(p.xy+vec2(0.0,-4.0))-0.1);
    vec2 ruler2pos = p.xy+vec2(-2.5,0.0);
    float ruler2_outline = min(half_ring(ruler2pos, 3.3, 0.7), square(ruler2pos+vec2(0.4,0.0), vec2(0.7,4.0)));
    float ruler2 = sdProfile(vec2(p.z,-ruler2_outline))-0.05;
    return max(min(ruler, ruler2),-ruler_hole);
}

float stress_blob(vec2 p) {
    return pow(max(1.0-length(p),0.0),4.0);
}

float stress(vec3 p) {
    return (0.5+0.5*sin(p.x*2.0))*0.25 + (0.5+0.5*cos(p.y*0.5))*0.25
         + max(1.0-sdLine(p.xy, vec2(0.0,3.7), vec2(0.0,-3.7)),0.0)
         + max(1.0-sdLine(p.xy, vec2(2.2,3.0), vec2(2.2,-3.0)),0.0)
         + max(min(1.0-abs(length(p.xy+vec2(-2.5,0))-3.3),p.x-2.0),0.0)
         + stress_blob(p.xy+vec2(0.0,3.3))*2.0
         + stress_blob(p.xy+vec2(0.0,2.7))*2.0
         + stress_blob(p.xy+vec2(0.3,-4.0))*3.0
         + stress_blob(p.xy+vec2(-0.3,-4.0))*3.0
         + stress_blob(p.xy+vec2(-2.8,-2.6))*4.0
         + stress_blob(p.xy+vec2(-2.8,2.6))*4.0
         + stress_blob(p.xy+vec2(-1.5,-4.0))
         + stress_blob(p.xy+vec2(-1.5,4.0))
         - stress_blob(p.xy+vec2(0.3,3.0))
         - stress_blob(p.xy+vec2(-0.3,3.0));
}

vec3 sceneGrad(vec3 point) {
    float t = scene(point);
    return normalize(vec3(
        t - scene(point + vec3(0.0001,0.0,0.0)),
        t - scene(point + vec3(0.0,0.0001,0.0)),
        t - scene(point + vec3(0.0,0.0,0.0001))));
}

bool castRay(inout Ray ray) {
    // Cast ray from origin into scene
    float sgn = sign(scene(ray.m_origin));
    for (int i = 0; i < 100; i++) {
        float dist = length(ray.m_point - ray.m_origin);
        if (dist > 40.0) {
            return false;
        }

        float smpl = scene(ray.m_point);
        
        if (abs(smpl) < 0.0001) {
            return true;
        }
        
        if (sgn < 0.0) ray.m_lag -= stress(ray.m_point)*smpl*5.0;
        
        ray.m_point += smpl * ray.m_direction * sgn;
    }
    ray.m_attenuation*=0.0;
    return false;
}

vec3 backlight(vec3 dir, float lag) {
    return ML(0.3+polarized*lag)*3.0*smoothstep(0.0,1.0,clamp(-0.8-dir.z*4.0,0.0,1.0));
}

void phongShadeRay(inout Ray ray) {
        vec3 normal = -sceneGrad(ray.m_point);

        vec3 reflected = reflect(ray.m_direction, normal);
        float frensel = abs(dot(ray.m_direction, normal));
        //oh god blackle clean this up
        ray.m_color += backlight(reflected,ray.m_lag)*0.8* (1.0 - frensel*0.98)*ray.m_attenuation;
        if (normal.z < -0.99) {
	        ray.m_attenuation*=texture(tex,ray.m_point.yx/vec2(8.0, 16.0)-vec2(0.5,0.5)).x;
            // ray.m_attenuation*=0.0;
        }
        // if (normal.z < -0.99 && sin(ray.m_point.y*60.0)>0.8 && ray.m_point.x<-0.6+(sin(ray.m_point.y*60.0/5.0)>0.5?0.1:0.0)) {
        //     ray.m_attenuation*=0.0;
        // }
}

void transmitRay(inout Ray ray) {
	float ior = 1.4;
    float sgn = sign(scene(ray.m_origin));
    vec3 normal = -sgn*sceneGrad(ray.m_point);
    //float frensel = clamp(abs(dot(ray.m_direction, normal))+0.5, 0.0,1.0); //this equation is bullshit
    ray.m_point -= normal*0.0005;
    
    ray.m_direction = refract(ray.m_direction, normal, sgn>0.0?1.0/ior:ior);
    float frensel = abs(dot(ray.m_direction, normal));
    ray.m_attenuation *= exp(-length(ray.m_point-ray.m_origin)*0.01)/exp(0.0);

    ray.m_origin=ray.m_point;
}

void recursivelyRender(inout Ray ray) {
    for (int i = 0; i < MAXDEPTH; i++) {
        if (castRay(ray)) {
            phongShadeRay(ray);
            transmitRay(ray);
        } else {
            ray.m_color += (i==0?pow(1.0-polarized,2.0):1.0)*backlight(ray.m_direction, ray.m_lag)*ray.m_attenuation;
			return;
        }
    }
}

vec2 Nth_weyl(int n) {
    return fract(vec2(n*12664745, n*9560333)/exp2(24.));
}

void main() {
	polarized = smoothstep(0.0, 1.0,sqrt(clamp((iTime-4.0)*1.5, 0.0, 1.0)));
	// Normalized pixel coordinates (from -1 to 1)
	vec2 uv_base = (gl_FragCoord.xy - vec2(960.0, 540.0))/vec2(960.0, 960.0);
	float pixelsize = 1.0/1920.0*2.5;

	vec3 cameraOrigin = vec3(4.0*sin(iTime*0.5), 4.0*cos(iTime*0.5), 5.0*abs(sin(iTime*0.25)))*4.0;
	vec3 focusOrigin = vec3(0.0, 0.0, 0.3);
	vec3 cameraDirection = normalize(focusOrigin-cameraOrigin);

	vec3 up = vec3(0.0,0.0,-1.0);
	vec3 plateXAxis = normalize(cross(cameraDirection, up));
	vec3 plateYAxis = normalize(cross(cameraDirection, plateXAxis));

	float fov = radians(40.0);

    vec3 col = vec3(0.0);
    
    for(int i = 0; i < SAMPLES;i++){
   		vec2 uv = uv_base + Nth_weyl(i)*pixelsize;
		vec3 platePoint = (plateXAxis * -uv.x + plateYAxis * uv.y) * tan(fov /2.0);

   		Ray ray = Ray(cameraOrigin, normalize(platePoint + cameraDirection), cameraOrigin, vec3(0.0), 1.0, 0.0);
    	recursivelyRender(ray);
    	col+= ray.m_color/float(SAMPLES);
    }

    col *= pow(max(1.0 - pow(length(uv_base)*0.7, 4.0), 0.0),3.0); 
    
    fragCol = vec4(pow(log(max(col+vec3(0.01,0.01,0.02),0.0)*.7+1.0),vec3(0.6))*0.9, 1.0);
		//fragCol = alphablend(alphablend(vec4(0.5, sin(uv*10.0 + sin(iTime)*2.0), 1.0), OSD(uv/2.0+vec2(0.5,0.0), iTime/3.0)),texture(tex, mix(vec2(-0.0,-0.1), vec2(0.2,0.1), (uv*0.5+0.3))).xxyz);
}