#include "ukf.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {

  index_ = 1;
  previous_timestamp_ = 0;
  is_initialized_ = false;
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);
 

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 3;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.6;

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;


  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */


  // State Dimension
  n_x_ = 5;

  // Augmented State Dimension
  n_aug_ = 7;

  // Sigma point spreading parameter
  lambda_ = 3 - n_aug_;

  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);

  //Weights for Sigma mean prediction
  weights_ = VectorXd(2 * n_aug_ + 1);
  double weight_0 = lambda_ / (lambda_ + n_aug_);
  weights_(0) = weight_0;
  for (int i = 1;i < 2 * n_aug_ + 1; i++) {
    double weight = 0.5 / (n_aug_ + lambda_);
    weights_(i) = weight;
  }
  //cout << "Weights \n " << weights_ << endl;

  P_ << 1,0,0,0,0,
    0,1,0,0,0,
    0,0,1000,0,0,
    0,0,0,1000,0,
    0,0,0,0,1000;


}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
  if(!is_initialized_) {
    cout <<"UKF: " << endl;
    x_ << 1,1,1,1,1;

    float px,py;

    if(meas_package.sensor_type_ == MeasurementPackage::RADAR) {

      float ro = meas_package.raw_measurements_[0];
      float phi = meas_package.raw_measurements_[1];
      px = ro * cos(phi);
      py = ro * sin(phi);
    }
    else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {

      px = meas_package.raw_measurements_[0];
      py = meas_package.raw_measurements_[1];
    }
    
    x_ << px,py,0,0,0;
    is_initialized_ = true;
    previous_timestamp_ = meas_package.timestamp_;

    //cout << "Initial x_ :\n" << x_<<endl;
 
    return;
  }

  double dt = (meas_package.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds
  //cout << "dt = " << dt << endl;
  previous_timestamp_ = meas_package.timestamp_;
 
  Prediction(dt);
  //cout << "Index : " << index_ << endl;
  //cout << " After prediction x: \n " << x_<<endl;
  //cout << " After prediction p: \n " << P_<<endl;

  if(meas_package.sensor_type_ == MeasurementPackage::RADAR) {
    UpdateRadar(meas_package);
  }

  else if(meas_package.sensor_type_ == MeasurementPackage::LASER) {
    UpdateLidar(meas_package);
  }
  //cout << " After update x: \n " << x_<<endl<<endl;
  //cout << " After update p: \n " << P_<<endl<<endl;

  index_ += 1;
  //  cout << " State : \n" << x_ << endl;
  //cout << " Cov : \n" << P_ << endl;
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */

  //cout << "yo = " << x_(3) << endl;
  MatrixXd Xsig_out = MatrixXd(n_aug_,2 * n_aug_ + 1);
  AugmentedSigmaPoints(&Xsig_out);
  SigmaPointPrediction(Xsig_out,delta_t);
  PredictMeanAndCovariance();
  
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */

  
  n_z_ = 2;
  MatrixXd Zsig = MatrixXd(n_z_, 2 * n_aug_ + 1);
  VectorXd z_pred = VectorXd(n_z_);
  MatrixXd S = MatrixXd(n_z_,n_z_);
  VectorXd z = VectorXd(n_z_);
  z << meas_package.raw_measurements_[0],meas_package.raw_measurements_[1];
  PredictLidarMeasurement(&z_pred,&S,&Zsig);
  UpdateLidarState(z,z_pred,S,Zsig);
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */

  n_z_ = 3;
  MatrixXd Zsig = MatrixXd(n_z_, 2 * n_aug_ + 1);
  VectorXd z_pred = VectorXd(n_z_);
  MatrixXd S = MatrixXd(n_z_,n_z_);
  VectorXd z = VectorXd(n_z_);
  z << meas_package.raw_measurements_[0],meas_package.raw_measurements_[1],meas_package.raw_measurements_[2];
  PredictRadarMeasurement(&z_pred,&S,&Zsig);
  UpdateRadarState(z,z_pred,S,Zsig);
  
}

/*
 * Generate Augmented Sigma Points
 */
void UKF::AugmentedSigmaPoints(MatrixXd* Xsig_out) {

  //Create Augmented Mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  //Create Augmented State Covariance
  MatrixXd P_aug = MatrixXd(n_aug_,n_aug_);

  //Create Sigma Point Matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_,2 * n_aug_ + 1);

  //Create Augmented Mean State
  x_aug.head(n_x_) = x_;
  x_aug(n_x_) = 0; // Mean value of longitudinal acceleration noise is 0
  x_aug(n_x_ + 1) = 0;// Mean value of radial acceleration noise is 0

  //Create Augmented Covariance Matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_ * std_a_;
  P_aug(6,6) = std_yawdd_ * std_yawdd_;

  //Create Square Root Matrix
  MatrixXd L = P_aug.llt().matrixL();

  //Create Augmented Sigma Points
  Xsig_aug.col(0) = x_aug;
  for(int i = 0; i < n_aug_ ; i++) {
    Xsig_aug.col(i+1) = x_aug + sqrt(lambda_ + n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_ + n_aug_) * L.col(i);
  }
  *Xsig_out = Xsig_aug;
  
}

/*
 * Sigma Point Prediction
 */
void UKF::SigmaPointPrediction(MatrixXd Xsig_aug,double delta_t) {
  //cout << "Delta T : " << delta_t<<endl;
  for(int i = 0; i < 2 *n_aug_+1;i++) {
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(0,2);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    //Predicted state values
    double px_p, py_p;

    //avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    }
    else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    
    //add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
   
    yawd_p = yawd_p + nu_yawdd*delta_t;
    
    //write predicted sigma point into right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;
    //cout << " i : " << i <<endl;
    //cout << "Augmented sigma point : \n" << Xsig_aug.col(i) <<endl;
    //cout << "Predicted sigma point : \n" << Xsig_pred_.col(i) <<endl;
  } 
}

/*
  Mean and Covariance of the Predicted State
 */
void UKF::PredictMeanAndCovariance() {

  //Create Vector for Predicted state
  VectorXd x = VectorXd(n_x_);

  //Create covariance matrix for prediction
  MatrixXd P = MatrixXd(n_x_,n_x_);

  //Predicted state mean
  x.fill(0.0);
  for (int i = 0 ; i < 2 * n_aug_ + 1; i++) {
    x = x + weights_(i) * Xsig_pred_.col(i);
  }

  
  //Predicted state covariance matrix
  P.fill(0.0);
  for(int i = 0; i < 2 * n_aug_ + 1;i++) {

    // State Difference
    VectorXd x_diff = Xsig_pred_.col(i) - x;

    //angle normalization
    //cout << "Entered : i :" << i << endl;
    //cout << " x_diff " << Xsig_pred_.col(i) << endl;

    x_diff(3) = NormalizeAngle(x_diff(3));
    x_diff(4) = NormalizeAngle(x_diff(4));
   
    //cout << " Exit : " << endl;
    P = P + weights_(i) * x_diff * x_diff.transpose() ; 
  }
  x_ = x;
  P_ = P;
  
}


/*
 * Transform prediction to radar measurement space
 */
void UKF::PredictRadarMeasurement(VectorXd* z_out,MatrixXd* S_out,MatrixXd* Zsig_out) {

  MatrixXd Zsig = MatrixXd(n_z_, 2 * n_aug_ + 1);

  for(int i = 0; i < 2 * n_aug_ + 1; i++) {

    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    //Measurement model
    Zsig(0,i) = sqrt(p_x * p_x + p_y * p_y);
    Zsig(1,i) = atan2(p_y,p_x);

    if (fabs(sqrt(p_x*p_x + p_y*p_y) > 0.001)) {
	Zsig(2,i) = (p_x * v1 + p_y * v2) / sqrt(p_x*p_x + p_y*p_y);
      }
    else {
      Zsig(2,i) = 0.0;
    }
  }

  //Mean predicted measurement
  VectorXd z_pred = VectorXd(n_z_);
  z_pred.fill(0.0);
  for(int i = 0;i < 2 * n_aug_ + 1;i++) {
    z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  //Measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z_,n_z_);
  S.fill(0.0);
  for(int i = 0; i < 2 * n_aug_ + 1; i++) {
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //angle normalization
  
    z_diff(1) = NormalizeAngle(z_diff(1));

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z_,n_z_);
  R <<    std_radr_*std_radr_, 0, 0,
    0, std_radphi_*std_radphi_, 0,
    0, 0,std_radrd_*std_radrd_;
  S = S + R;

  *z_out = z_pred;
  *S_out = S;
  *Zsig_out = Zsig;
}

/*
 * Transform prediction to radar measurement space
 */
void UKF::PredictLidarMeasurement(VectorXd* z_out,MatrixXd* S_out,MatrixXd* Zsig_out) {

  MatrixXd Zsig = MatrixXd(n_z_, 2 * n_aug_ + 1);

  for(int i = 0; i < 2 * n_aug_ + 1; i++) {

    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);

    //Measurement model
    Zsig(0,i) = p_x;
    Zsig(1,i) = p_y;
  }

  //Mean predicted measurement
  VectorXd z_pred = VectorXd(n_z_);
  z_pred.fill(0.0);
  for(int i = 0;i < 2 * n_aug_ + 1;i++) {
    z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  //Measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z_,n_z_);
  S.fill(0.0);
  for(int i = 0; i < 2 * n_aug_ + 1; i++) {
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z_,n_z_);
  R <<    std_laspx_*std_laspx_, 0,
    0,std_laspy_*std_laspy_;
  S = S + R;

  *z_out = z_pred;
  *S_out = S;
  *Zsig_out = Zsig;
}


void UKF::UpdateRadarState(VectorXd z,VectorXd z_pred,MatrixXd S,MatrixXd Zsig) {

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z_);

   //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    
    //angle normalization
  
    z_diff(1) = NormalizeAngle(z_diff(1));
 
    
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
 
    x_diff(3) = NormalizeAngle(x_diff(3));
    x_diff(4) = NormalizeAngle(x_diff(4));
   

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff = z - z_pred;

  //angle normalization
 
  z_diff(1) = NormalizeAngle(z_diff(1));

  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;

  x_(3) = NormalizeAngle(x_(3));
  x_(4) = NormalizeAngle(x_(4));
 
  P_ = P_ - K*S*K.transpose();

  
  NIS_radar_ = z_diff.transpose() * S.inverse() * z_diff;
  //cout << "Radar NIS :" << NIS_radar_ << endl;
}


void UKF::UpdateLidarState(VectorXd z,VectorXd z_pred,MatrixXd S,MatrixXd Zsig) {

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z_);

   //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;

    
    //angle normalization
 
    x_diff(3) = NormalizeAngle(x_diff(3));
    x_diff(4) = NormalizeAngle(x_diff(4));
   
    
    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff = z - z_pred;

  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;

  x_(3) = NormalizeAngle(x_(3));
  x_(4) = NormalizeAngle(x_(4));
  
  P_ = P_ - K*S*K.transpose();
  

  NIS_laser_ = z_diff.transpose() * S.inverse() * z_diff;
  //cout << "Laser NIS :" << NIS_laser_ << endl;
}

double UKF::NormalizeAngle(double angle) {
    return atan2(sin(angle),cos(angle));
}
