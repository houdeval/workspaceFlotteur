from math import *
import numpy as np
import matplotlib.pyplot as plt

g = 9.81
rho = 1025.0 # kg/m3
m=8.3
Cf = np.pi*(0.12)**2

A=g*rho/m
B=0*0.5*rho*Cf/m
tick_to_volume = (1.75e-3/48.0)*((0.05/2.0)**2)*np.pi

alpha = -0.*tick_to_volume

x1=0.
x2=0.
x3=0.
v_error = 50.*tick_to_volume
x3_estim = x3 + v_error
dt=0.01
set_point=5.

u=0.

xhat = np.array([[0],[0],[0],[0]])
gamma = 10000*np.eye(4)
gamma_alpha = np.diag([1e-4, 1e-6, 1e-6, 1e-10])
gamma_beta = np.diag([1e-6, 1e-20])


def kalman_predict(xup,Gup,u,gamma_alpha,A):
    gamma_1 = A @ Gup @ A.T + gamma_alpha
    x1 = A @ xup + u    
    return(x1,gamma_1)

def kalman_correc(x0,gamma_0,y,gamma_beta,C):
    S = C @ gamma_0 @ C.T + gamma_beta        
    K = gamma_0 @ C.T @ np.linalg.inv(S)           
    ytilde = y - C @ x0   
    Gup = (np.eye(len(x0))-K @ C) @ gamma_0
    xup = x0 + K@ytilde
    return(xup,Gup) 
    
def kalman(x0,gamma_0,u,y,gamma_alpha,gamma_beta,A,C):
    xup,Gup = kalman_correc(x0,gamma_0,y,gamma_beta,C)
    x1,gamma_1=kalman_predict(xup,Gup,u,gamma_alpha,A)
    return(x1,gamma_1)     


def euler(u):
	global x1, x2, x3
	x1= x1+(-A*(x3-alpha*x2)-B*x1**2*np.sign(x1))*dt
	x2= x2+x1*dt
	x3= x3+u*dt

def control():
	global xhat, u, gamma, gamma_alpha, gamma_beta, x2, x3_estim
	# m=8.3
	# Cf = 1.1*np.pi*(0.12)**2
	# A=g*rho/m
	# B=0.5*rho*Cf/m
	xhat1, xhat2, xhat3, xhat4 = xhat.flatten()
	Ak = np.eye(4)+dt*np.array([[0, A*alpha, -A, -A],
								[1, 0, 0, 0],
								[0, 0, 0, 0],
								[0, 0, 0, 0]])
	Ck = np.array([[0,1,0,0], [0,0,1,0]])
	measure = np.array([[x2],[x3 + v_error]]) + gamma_beta @ np.random.randn(2,1)
	#zk = measure - np.array([[xhat2]])
	xhat, gamma = kalman(xhat,gamma,np.array([[0],[0],[u],[0]]),measure,gamma_alpha,gamma_beta,Ak,Ck)

	x3_estim = xhat[2][0] - xhat[3][0]

	#print("estime : " + str(xhat))
	#print("reel : ", x1, x2, x3, x3_estim-x3)

	beta = -0.02*np.pi/2.0
	l = 100.

	e=set_point-x2
	dx1 = -A*x3_estim-B*(x1**2)*np.sign(x1)

	y = x1 + beta * atan(e)
	dy = dx1-beta*x1/(1.+e**2)

	u = (1./A)*(-2.*B*x1*dx1*np.sign(x1) -beta*(2.*(x1**2)*e + dx1*(1.+e**2))/((1.+e**2)**2) +l*dy+y)
	ddy = -A*u-2.*B*x1*dx1*np.sign(x1)-beta*(2*x1**2*e+dx1*(1.+e**2))/((1+e**2)**2)

	eq = y+l*dy+ddy

	y_log.append(y)
	dy_log.append(dy)
	ddy_log.append(ddy)
	eq_log.append(y+l*dy+ddy)

	return u

x1_log = [x1]
x2_log = [x2]
x3_log = [x3]
x4_log = [v_error]
y_log = []
dy_log = []
ddy_log = []
eq_log = []

xhat1_log = []
xhat2_log = []
xhat3_log = []
xhat4_log = []

for i in range(100000):
	# euler()
	u = control()
	
	euler(u)
	
	x1_log.append(x1)
	x2_log.append(x2)
	x3_log.append(x3)
	x4_log.append(v_error)
	
	xhat1_log.append(xhat[0][0])
	xhat2_log.append(xhat[1][0])
	xhat3_log.append(xhat[2][0])
	xhat4_log.append(xhat[3][0])

print(gamma)

fig, (ax1, ax2, ax3, ax4) = plt.subplots(4,1, sharex=True)
ax1.set_ylabel('x1')
ax1.plot(x1_log)
ax1.plot(xhat1_log, 'r')
ax2.set_ylabel('x2')
ax2.plot(x2_log)
ax2.plot(xhat2_log, 'r')
ax3.set_ylabel('x3')
ax3.plot(x3_log)
ax3.plot(xhat3_log, 'r')
ax4.set_ylabel('x4')
ax4.plot(x4_log)
ax4.plot(xhat4_log, 'r')

#fig, (ax1, ax2, ax3, ax4) = plt.subplots(4,1, sharex=True)
#ax1.set_ylabel('y')
#ax1.plot(y_log)
#ax2.set_ylabel('dy')
#ax2.plot(dy_log)
#ax3.set_ylabel('ddy')
#ax3.plot(ddy_log)
#ax4.set_ylabel('eq')
#ax4.plot(eq_log)

plt.show()