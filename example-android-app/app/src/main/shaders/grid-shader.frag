// Pristine grid from The Best Darn Grid Shader (yet)
// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8
//
// https://www.shadertoy.com/view/mdVfWw

precision highp float;
// #extension GL_OES_standard_derivatives : enable

float pristineGrid(in vec2 uv, vec2 lineWidth) {
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);
    vec2 uvDeriv = vec2(length(vec2(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
    bvec2 invertLine = bvec2(lineWidth.x > 0.5, lineWidth.y > 0.5);
    vec2 targetWidth = vec2(
        invertLine.x ? 1.0 - lineWidth.x : lineWidth.x,
        invertLine.y ? 1.0 - lineWidth.y : lineWidth.y
    );
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
    gridUV.y = invertLine.y ? gridUV.y : 1.0 - gridUV.y;
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);

    grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
    grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
    grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;
    return mix(grid2.x, 1.0, grid2.y);
}

// version with explicit gradients for use with raycast shaders like this one
float pristineGrid(in vec2 uv, in vec2 ddx, in vec2 ddy, vec2 lineWidth) {
    vec2 uvDeriv = vec2(length(vec2(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
    vec2 drawWidth = clamp(lineWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = vec2(1.0, 1.0) - abs(fract(uv) * 2.0 - 1.0);
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= clamp(lineWidth / drawWidth, 0.0, 1.0);
    grid2 = mix(grid2, lineWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
    return mix(grid2.x, 1.0, grid2.y);
}

#define BACKGROUND_VAL 0.0

const float N = 10.0; // grid ratio

// sphere implementation
float softShadowSphere(in vec3 ro, in vec3 rd, in vec4 sph) {
    vec3 oc = sph.xyz - ro;
    float b = dot(oc, rd);

    float res = 1.0;
    if (b > 0.0) {
        float h = dot(oc,oc) - b*b - sph.w*sph.w;
        res = smoothstep( 0.0, 1.0, 2.0*h/b );
    }
    return res;
}

float occSphere(in vec4 sph, in vec3 pos, in vec3 nor) {
    vec3 di = sph.xyz - pos;
    float l = length(di);
    return 1.0 - dot(nor, di/l) * sph.w * sph.w / (l * l);
}

float iSphere(in vec3 ro, in vec3 rd, in vec4 sph) {
    float t = -1.0;
    vec3  ce = ro - sph.xyz;
    float b = dot(rd, ce);
    float c = dot(ce, ce) - sph.w * sph.w;
    float h = b*b - c;
    if (h > 0.0) {
        t = -b - sqrt(h);
    }
    return t;
}

// Spheres
const vec4 spheres[4] = vec4[4](
    vec4(  3.0,-2.5, 0.0, 0.0 ),
    vec4( -4.0, 2.0,-5.0, 2.0 ),
    vec4( -4.0, 2.0, 5.0, 2.0 ),
    vec4(-30.0, 8.0, 0.0, 8.0 )
);

float intersect(vec3 ro, vec3 rd, out vec3 pos, out vec3 nor, out float occ, out int intersected_entity_id)
{
    // raytrace
    float tmin = 10000.0;
    nor = vec3(0.0);
    occ = 1.0;
    pos = vec3(0.0);
    intersected_entity_id = -1;

    // raytrace-plane
    float h = (0.01 - ro.y) / rd.y;
    if (h > 0.0) {
        tmin = h;
        nor = vec3(0.0,1.0,0.0);
        pos = ro + h*rd;
        intersected_entity_id = 0;
        occ = 1.0;
        for (int i = 0; i < 4; i++) {
            occ *= occSphere(spheres[i], pos, nor);
        }
    }

    // raytrace-sphere
    for (int i = 0; i < 4; i++) {
        vec4 sphere = spheres[i];
        h = iSphere(ro, rd, sphere);
        if (h > 0.0 && h < tmin) {
            tmin = h;
            pos = ro + h * rd;
            nor = normalize(pos - sphere.xyz);
            intersected_entity_id = i + 1;
            occ = 0.5 + 0.5 * nor.y;
        }
    }

    return tmin;
}

vec2 texCoords(in vec3 pos, int mid) {
    vec2 matuv;
    if (mid == 0) {
        matuv = pos.xz;
    } else {
        vec4 sphere = spheres[mid - 1];
        vec3 q = normalize(pos - sphere.xyz);
        matuv = vec2(atan(q.x, q.z), acos(q.y)) * sphere.w;
    }
    return 8.0 * matuv;
}

void calcCamera(out vec3 ro, out vec3 ta) {
    float an = 0.1 * sin(0.1 * iTime);
    ro = vec3(5.0 * cos(an), 0.5, 5.0 * sin(an));
    ta = vec3(0.0, 1.0, 0.0);
}

vec3 doLighting(in vec3 pos, in vec3 nor, in float occ, in vec3 rd) {
    float sh = min(min(min(softShadowSphere(pos, vec3(0.57703), spheres[0]),
                           softShadowSphere(pos, vec3(0.57703), spheres[1])),
                           softShadowSphere(pos, vec3(0.57703), spheres[2])),
                           softShadowSphere(pos, vec3(0.57703), spheres[3]));
    float dif = clamp(dot(nor, vec3(0.57703)), 0.0, 1.0);
    float bac = clamp(0.5 + 0.5 * dot(nor, vec3(-0.707, 0.0, -0.707)), 0.0, 1.0);
    vec3 lin = dif*vec3(1.50, 1.40, 1.30) * sh;
    lin += occ * vec3(0.15, 0.20, 0.30);
    lin += bac * vec3(0.10, 0.10, 0.10) * (0.2 + 0.8 * occ);
    return lin;
}
//===============================================================================================
//===============================================================================================
// render
//===============================================================================================
//===============================================================================================

void calcRayForPixel(in vec2 pix, out vec3 resRo, out vec3 resRd) {
    vec2 p = (2.0 * pix - iResolution.xy) / iResolution.y;

    // camera movement
    vec3 ta;
    calcCamera(resRo, ta);
    // camera matrix
    vec3 ww = normalize(ta - resRo);
    vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    // create view ray
    resRd = normalize(p.x * uu + p.y * vv + 2.0 * ww);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = (-iResolution.xy + 2.0 * gl_FragCoord.xy) / iResolution.y;
    float th = (iMouse.z > 0.001) ? (2.0 * iMouse.x-iResolution.x) / iResolution.y : 0.0;

    vec3 ro, rd, ddx_ro, ddx_rd, ddy_ro, ddy_rd;
    calcRayForPixel(gl_FragCoord.xy + vec2(0.0, 0.0), ro, rd);
    calcRayForPixel(gl_FragCoord.xy + vec2(1.0, 0.0), ddx_ro, ddx_rd);
    calcRayForPixel(gl_FragCoord.xy + vec2(0.0, 1.0), ddy_ro, ddy_rd);

    // trace
    vec3 pos, nor;
    float occ;
    int mid;
    float t = intersect(ro, rd, pos, nor, occ, mid);

    vec3 col = vec3(BACKGROUND_VAL);
    if (mid != -1) {
        #if 1
		// -----------------------------------------------------------------------
        // compute ray differentials by intersecting the tangent plane to the
        // surface.
        // -----------------------------------------------------------------------

        // computer ray differentials
        vec3 ddx_pos = ddx_ro - ddx_rd * dot(ddx_ro-pos,nor) / dot(ddx_rd, nor);
        vec3 ddy_pos = ddy_ro - ddy_rd * dot(ddy_ro-pos,nor) / dot(ddy_rd, nor);

        // calc texture sampling footprint
        vec2     uv = texCoords(    pos, mid);
        vec2 ddx_uv = texCoords(ddx_pos, mid) - uv;
        vec2 ddy_uv = texCoords(ddy_pos, mid) - uv;
        #else
		// -----------------------------------------------------------------------
        // Because we are in the GPU, we do have access to differentials directly
        // This wouldn't be the case in a regular raytracer.
        // It wouldn't work as well in shaders doing interleaved calculations in
        // pixels (such as some of the 3D/stereo shaders here in Shadertoy)
        // -----------------------------------------------------------------------
        vec2 uvw = texCoords(pos, mid);

        // calc texture sampling footprint
        vec2 ddx_uvw = dFdx( uvw );
        vec2 ddy_uvw = dFdy( uvw );
        #endif

        // shading
        vec3 mate = 1.0 - vec3(1.0)*(1.0 - pristineGrid(uv - vec2(0.05), ddx_uv, ddy_uv, vec2(1.0 / N)));

        // lighting
        vec3 lin = doLighting( pos, nor, occ, rd );

        // combine lighting with material
        col = mate * lin;

        // fog
        col = mix(col, vec3(BACKGROUND_VAL), 1.0 - exp(-0.00001 * t * t));
    }

    // gamma correction
    col = pow(col, vec3(0.4545)) * smoothstep(1.0, 2.0, abs(p.x-th) / (2.0 / iResolution.y));

    fragColor = vec4(col, 1.0);
}