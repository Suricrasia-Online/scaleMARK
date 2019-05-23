#!/usr/bin/env python3

import numpy as np
import scipy.integrate as integrate
import scipy.optimize as optimize
import scipy.misc as misc

def interp_func(vx, vy):
	return lambda x : np.interp(x, vx, vy, 0, 0)

def spectral_dot(func1, func2):
	# spectral integration ranges
	xmin = 380
	xmax = 780
	return integrate.fixed_quad(lambda x : func1(x) * func2(x), xmin, xmax, n=5000)[0]

def build_spectral_to_xyz():
	color_matching_data = np.genfromtxt('lin2012xyz2e_1_7sf.csv', delimiter=',')
	color_matching_x = interp_func(color_matching_data[:,0], color_matching_data[:,1])
	color_matching_y = interp_func(color_matching_data[:,0], color_matching_data[:,2])
	color_matching_z = interp_func(color_matching_data[:,0], color_matching_data[:,3])

	return lambda spectral: np.array([spectral_dot(spectral, color_matching_x), spectral_dot(spectral, color_matching_y), spectral_dot(spectral, color_matching_z)])

def xyz_to_linear_srgb(xyz):
	xyztosrgb = np.matrix([
		[ 3.2404542, -1.5371385, -0.4985314],
		[-0.9692660,  1.8760108,  0.0415560],
		[ 0.0556434, -0.2040259,  1.0572252]])
	return np.asarray(xyztosrgb.dot(xyz)).reshape(-1)

def normalize_rgb(rgb):
	if not np.all(rgb == 0):
		rgb /= np.max(rgb) #normalize (you're not normally supposed to do this...)
	return rgb

def linear_srgb_luminance(rgb):
	return np.average(rgb, weights=[0.213, 0.715, 0.072])

def linear_srgb_to_rgb(rgb):
	rgb = np.clip(rgb, 0, np.inf)
	nonlinearity = np.vectorize(lambda x: 12.92*x if x < 0.0031308 else 1.055*(x**(1.0/2.4))-0.055)
	return np.clip(nonlinearity(rgb), 0, 1)

def srgb_to_termstring(rgb, rgb2):
	rgb = (rgb*255).flat
	rgb2 = (rgb2*255).flat
	return "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm▌\x1b[0m" % (int(rgb[0]), int(rgb[1]), int(rgb[2]), int(rgb2[0]), int(rgb2[1]), int(rgb2[2]))

#from https://scipython.com/blog/converting-a-spectrum-to-a-colour/
from scipy.constants import h, c, k, pi
def planck(lam, T):
	""" Returns the spectral radiance of a black body at temperature T.

	Returns the spectral radiance, B(lam, T), in W.sr-1.m-2 of a black body
	at temperature T (in K) at a wavelength lam (in nm), using Planck's law.

	"""

	lam_m = lam / 1.e9
	fac = h*c/lam_m/k/T
	B = 2*h*c**2/lam_m**5 / (np.exp(fac) - 1)
	return B

def birefringence_lag(lag, lam):
	return np.sin(pi*lag/lam)**2

def spectrum_from_csv(csvfile):
	data = np.genfromtxt(csvfile, delimiter=',')
	return interp_func(data[:,0], data[:,1])

def group(lst, n):
  for i in range(0, len(lst), n):
    val = lst[i:i+n]
    if len(val) == n:
      yield tuple(val)

spectral_to_xyz = build_spectral_to_xyz()

def spectral_to_linear_srgb(spectral):
	return xyz_to_linear_srgb(spectral_to_xyz(spectral))

def birefringence_spectrum_for_blackbody(temp):
	spectrum = []
	lagrange = np.arange(0, 1200, 5)
	for lag in lagrange:
		color = spectral_to_linear_srgb(lambda x : birefringence_lag(lag, x)*planck(x, temp))
		spectrum.append(color)

	spectrum = np.matrix(spectrum)
	spectrum /= np.max(np.apply_along_axis(lambda x: linear_srgb_luminance(np.asarray(x).reshape(-1)), axis=1, arr=spectrum))

	x_fuc = interp_func(lagrange, np.asarray(spectrum[:,0]).reshape(-1))
	y_fuc = interp_func(lagrange, np.asarray(spectrum[:,1]).reshape(-1))
	z_fuc = interp_func(lagrange, np.asarray(spectrum[:,2]).reshape(-1))
	return lambda x: np.array([x_fuc(x), y_fuc(x), z_fuc(x)])

def approximation_for_vals(vals):
	def approximation(x):
		funcval = np.power(np.sin(np.outer(vals[0:3], x)), 2)

		mat = vals[3:].reshape((3,3)).T
		funcval = mat.dot(funcval)

		return funcval

	return approximation

def main():
	blackbody_12000K_func = birefringence_spectrum_for_blackbody(12000)

	x0 = [0.00509906, 0.00580413, 0.00707829,
		1, 0, 0,
		0, 1, 0,
		0, 0, 1]

	def objective(vals):
		approx = approximation_for_vals(vals)
		def integrand(x):
			standard = np.linalg.norm(blackbody_12000K_func(x) - approx(x), np.inf)
			deriv = np.linalg.norm(misc.derivative(blackbody_12000K_func, x, dx=1e-6) - misc.derivative(approx, x, dx=1e-6), np.inf)
			return standard + deriv*200.0
		return integrate.fixed_quad(integrand, 0, 1200, n=10000)[0]

	result = optimize.minimize(objective, np.array(x0), method='Powell', options={'maxiter': 1000})
	start_approx = approximation_for_vals(x0)
	final_approx = approximation_for_vals(result.x)
	print(result)

	def print_birefringence_spectrum(formula):
		spectrum = []
		maxcol = 0
		for lag in np.arange(0, 1200, 5):
			spectrum.append(formula(lag))

		spectrum_string = ""
		for color1, color2 in group(spectrum, 2):
			spectrum_string += srgb_to_termstring(linear_srgb_to_rgb(color1), linear_srgb_to_rgb(color2))
		print(spectrum_string)
		print(spectrum_string)

	print_birefringence_spectrum(blackbody_12000K_func)
	print_birefringence_spectrum(final_approx)


if __name__ == "__main__":
	main();
