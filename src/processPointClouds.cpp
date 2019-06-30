// PCL lib Functions for processing point clouds 

#include "processPointClouds.h"


//constructor:
template<typename PointT>
ProcessPointClouds<PointT>::ProcessPointClouds() {}


//de-constructor:
template<typename PointT>
ProcessPointClouds<PointT>::~ProcessPointClouds() {}


template<typename PointT>
void ProcessPointClouds<PointT>::numPoints(typename pcl::PointCloud<PointT>::Ptr cloud)
{
    std::cout << cloud->points.size() << std::endl;
}


template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr ProcessPointClouds<PointT>::FilterCloud(typename pcl::PointCloud<PointT>::Ptr cloud, float filterRes, Eigen::Vector4f minPoint, Eigen::Vector4f maxPoint)
{
    // Time filtering process
    auto startTime = std::chrono::steady_clock::now();
    typename pcl::PointCloud<PointT>::Ptr filteredCloud{new pcl::PointCloud<PointT>};

    // Filter point cloud
    pcl::VoxelGrid<PointT> sor;
    sor.setInputCloud (cloud);
    sor.setLeafSize (filterRes, filterRes, filterRes);
    sor.filter (*filteredCloud);

    // Crop the region of interest
    pcl::CropBox<PointT> roi;
    roi.setInputCloud(filteredCloud);
    roi.setMin(minPoint);
    roi.setMax(maxPoint);
    roi.filter(*filteredCloud);

    // Remove ego car points
    const Eigen::Vector4f minPointCar = {-1.5f, -1.5f, -1.0f, 1.0f};
    const Eigen::Vector4f maxPointCar = {2.7f, 1.5f, -0.4f, 1.0f};
    pcl::PointIndices::Ptr carPoints (new pcl::PointIndices());
    pcl::CropBox<PointT> roof;
    roof.setInputCloud(filteredCloud);
    roof.setMin(minPointCar);
    roof.setMax(maxPointCar);
    roof.setNegative(true);
    roof.filter(*filteredCloud);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "filtering took " << elapsedTime.count() << " milliseconds" << std::endl;

    return filteredCloud;

}


template<typename PointT>
std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SeparateClouds(pcl::PointIndices::Ptr inliers, typename pcl::PointCloud<PointT>::Ptr cloud) 
{
    // TODO: Create two new point clouds, one cloud with obstacles and other with segmented plane
    typename pcl::PointCloud<PointT>::Ptr plainCloud{new pcl::PointCloud<PointT>};
    typename pcl::PointCloud<PointT>::Ptr obstCloud{new pcl::PointCloud<PointT>};

    // Extract the inliers
    pcl::ExtractIndices<pcl::PointXYZ> extract;
    extract.setInputCloud (cloud);
    extract.setIndices (inliers);
    extract.setNegative (false);
    extract.filter (*plainCloud);

    // Create the filtering object
    extract.setNegative (true);
    extract.filter (*obstCloud);

    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult(obstCloud, plainCloud);
    return segResult;
}


template<typename PointT>
std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SegmentPlane(typename pcl::PointCloud<PointT>::Ptr cloud, int maxIterations, float distanceThreshold)
{
    // Time segmentation process
    auto startTime = std::chrono::steady_clock::now();
	srand(time(NULL));
	
	const size_t n = cloud->points.size();
    std::vector<int> tmp;
    tmp.reserve(n);
    std::vector<int> inliersResult;
    inliersResult.reserve(n);
	// TODO: Fill in this function
	// For max iterations 
	// Randomly sample subset and fit line
	// Measure distance between every point and fitted line
	// If distance is smaller than threshold count it as inlier
	// Return indicies of inliers from fitted line with most inliers
	for(int i = 0 ; i < maxIterations; ++i) {
		// select three points
		int p1 = std::rand() % n;
		int p2 = p1, p3 = p1;
		while (p1 == p2) {
			p2 = std::rand() % n;
		}

		while (p3 == p1 || p3 == p2) {
			p3 = std::rand() % n;
		}

        const auto & point1 = cloud->points[p1];
        const auto & point2 = cloud->points[p2];
        const auto & point3 = cloud->points[p3];

		const double x1 = point1.x, x2 = point2.x, x3 = point3.x;
		const double y1 = point1.y, y2 = point2.y, y3 = point3.y;
		const double z1 = point1.z, z2 = point2.z, z3 = point3.z;

		const double a = (y2 - y1)*(z3 - z1) - (z2 - z1)*(y3 - y1);
		const double b = (z2 - z1)*(x3 - x1) - (x2 - x1)*(z3 - z1);
		const double c = (x2 - x1)*(y3 - y1) - (y2 - y1)*(x3 - x1);
		const double d = -(a*x1 + b*y1 + c*z1);

		const double norm = std::sqrt(a*a + b*b + c*c);
        //std::cout << "Norm: " << norm << std::endl;
		tmp.clear();
		for (size_t j = 0; j < cloud->points.size(); ++j) {
			const auto& point = cloud->points[j];
			const double dist = std::abs(a*point.x + b*point.y + c*point.z + d) / norm;
        
			if (dist <= distanceThreshold) {
                //std::cout << "Dist: " << dist << std::endl;
				tmp.push_back(j);
			}
		}

		if (tmp.size() > inliersResult.size()) {
			swap(inliersResult, tmp);
		}
	}
    pcl::PointIndices::Ptr inliers (new pcl::PointIndices());
    inliers->indices = std::move(inliersResult);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "plane segmentation took " << elapsedTime.count() << " milliseconds" << std::endl;

    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult = SeparateClouds(inliers,cloud);
    return segResult;
}


template<typename PointT>
std::vector<typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::Clustering(typename pcl::PointCloud<PointT>::Ptr cloud, float clusterTolerance, int minSize, int maxSize)
{

    // Time clustering process
    auto startTime = std::chrono::steady_clock::now();

    std::vector<typename pcl::PointCloud<PointT>::Ptr> clusters;

    // TODO:: Fill in the function to perform euclidean clustering to group detected obstacles
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud (cloud);

    std::vector<pcl::PointIndices> cluster_indices;
    pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
    ec.setClusterTolerance(clusterTolerance);
    ec.setMinClusterSize(minSize);
    ec.setMaxClusterSize(maxSize);
    ec.setSearchMethod(tree);
    ec.setInputCloud(cloud);
    ec.extract (cluster_indices);

    for (auto it = cluster_indices.begin(); it != cluster_indices.end(); ++it)
    {
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZ>);
        for (auto pit = it->indices.begin(); pit != it->indices.end(); ++pit)
        {
            cloud_cluster->points.push_back(cloud->points[*pit]);
        }
        cloud_cluster->width = cloud_cluster->points.size();
        cloud_cluster->height = 1;
        cloud_cluster->is_dense = true;

        clusters.emplace_back(std::move(cloud_cluster));
    }

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "clustering took " << elapsedTime.count() << " milliseconds and found " << clusters.size() << " clusters" << std::endl;

    return clusters;
}


template<typename PointT>
Box ProcessPointClouds<PointT>::BoundingBox(typename pcl::PointCloud<PointT>::Ptr cluster)
{

    // Find bounding box for one of the clusters
    PointT minPoint, maxPoint;
    pcl::getMinMax3D(*cluster, minPoint, maxPoint);

    Box box;
    box.x_min = minPoint.x;
    box.y_min = minPoint.y;
    box.z_min = minPoint.z;
    box.x_max = maxPoint.x;
    box.y_max = maxPoint.y;
    box.z_max = maxPoint.z;

    return box;
}

template<typename PointT>
BoxQ ProcessPointClouds<PointT>::BoundingBoxQ(typename pcl::PointCloud<PointT>::Ptr cluster)
{
    // PCA
    Eigen::Vector4f pcaCentroid;
    pcl::compute3DCentroid(*cluster, pcaCentroid);
    Eigen::Matrix3f covariance;
    computeCovarianceMatrixNormalized(*cluster, pcaCentroid, covariance);
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigen_solver(covariance, Eigen::ComputeEigenvectors);
    Eigen::Matrix3f eigenVectorsPCA = eigen_solver.eigenvectors();
    // Ignore z rotation.
    // First rearrange eigen vector matrix
    // e3 vector will be a vector with max Z coordinate
    // We want to project e1 and e2 on XY plane, so if one of them haz max Z we should swap it with e3
    Eigen::Vector3f::Index idx;
    eigenVectorsPCA.row(2).cwiseAbs().maxCoeff(&idx);
    auto tmp = eigenVectorsPCA.col(idx);
    eigenVectorsPCA.col(idx) = eigenVectorsPCA.col(2);
    eigenVectorsPCA.col(2) = tmp;

    // We need to normalize e1 and e2 vecotors
    // puting their z coordinates equal to 0
    // This will give us the projection to of PCA vectors to the original XY plane
    // After that e3 vector is calculated as X product of prjected and normalize e1 and e2
    // This is required to correctly determine z vector direction
    // making e1, e2, e3 a right handed vector system
    eigenVectorsPCA.row(2) << 0, 0, 0;
    eigenVectorsPCA.col(0).normalize();
    eigenVectorsPCA.col(1).normalize();
    eigenVectorsPCA.col(2) = eigenVectorsPCA.col(0).cross(eigenVectorsPCA.col(1));
    eigenVectorsPCA.col(2).normalize();

    // Form transformation matrix.
    Eigen::Matrix4f projectionTransform(Eigen::Matrix4f::Identity());
    projectionTransform.block<3,3>(0,0) = eigenVectorsPCA.transpose();
    // Transform centroid coordinates
    projectionTransform.block<3,1>(0,3) = -1.f * (projectionTransform.block<3,3>(0,0) * pcaCentroid.head<3>());
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudPointsProjected (new pcl::PointCloud<pcl::PointXYZ>);
    // Transform the original cloud.
    pcl::transformPointCloud(*cluster, *cloudPointsProjected, projectionTransform);
    
    // Get the minimum and maximum points of the transformed cloud.
    pcl::PointXYZ minPoint, maxPoint;
    pcl::getMinMax3D(*cloudPointsProjected, minPoint, maxPoint);

    BoxQ box;
    box.cube_length = maxPoint.x - minPoint.x;
    box.cube_width = maxPoint.y - minPoint.y;
    box.cube_height = maxPoint.z - minPoint.z;

    // Bbox quaternion and transforms
    box.bboxQuaternion = Eigen::Quaternionf(eigenVectorsPCA);
    const Eigen::Vector3f meanDiagonal = 0.5f*(maxPoint.getVector3fMap() + minPoint.getVector3fMap());
    box.bboxTransform = eigenVectorsPCA * meanDiagonal + pcaCentroid.head<3>();

    return box;
}


template<typename PointT>
void ProcessPointClouds<PointT>::savePcd(typename pcl::PointCloud<PointT>::Ptr cloud, std::string file)
{
    pcl::io::savePCDFileASCII (file, *cloud);
    std::cerr << "Saved " << cloud->points.size () << " data points to "+file << std::endl;
}


template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr ProcessPointClouds<PointT>::loadPcd(std::string file)
{

    typename pcl::PointCloud<PointT>::Ptr cloud (new pcl::PointCloud<PointT>);

    if (pcl::io::loadPCDFile<PointT> (file, *cloud) == -1) //* load the file
    {
        PCL_ERROR ("Couldn't read file \n");
    }
    std::cerr << "Loaded " << cloud->points.size () << " data points from "+file << std::endl;

    return cloud;
}


template<typename PointT>
std::vector<boost::filesystem::path> ProcessPointClouds<PointT>::streamPcd(std::string dataPath)
{

    std::vector<boost::filesystem::path> paths(boost::filesystem::directory_iterator{dataPath}, boost::filesystem::directory_iterator{});

    // sort files in accending order so playback is chronological
    sort(paths.begin(), paths.end());

    return paths;

}