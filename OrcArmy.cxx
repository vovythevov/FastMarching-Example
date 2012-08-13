#include <iostream>
#include <time.h>

#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkFastMarchingImageFilter.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>

int main( int argc, char * argv[] )
{
  //
  //Create orc army image
  //

  //Classic typedefs
  typedef unsigned char PixelType;
  typedef itk::Image<PixelType, 2> ImageType;
  typedef itk::Image<float, 2> FloatImageType;

  //Start creat image
  ImageType::Pointer orcArmy = ImageType::New();
  ImageType::IndexType orcArmyStart = {0, 0};
  ImageType::SizeType orcArmySize = {20000, 20000};
  ImageType::RegionType orcArmyRegion;
  orcArmyRegion.SetIndex( orcArmyStart );
  orcArmyRegion.SetSize( orcArmySize );
  orcArmy->SetRegions( orcArmyRegion );

  //Put an origin
  ImageType::PointType orcArmyOrigin;
  orcArmyOrigin[0] = 42;//Orc haz weird or'gin
  orcArmyOrigin[1] = -42;
  orcArmy->SetOrigin( orcArmyOrigin );

  //Put a spacing
  ImageType::SpacingType orcArmySpacing;
  orcArmySpacing[0] = 0.3; //Orc army not squared
  orcArmySpacing[1] = 0.9;
  orcArmy->SetSpacing( orcArmySpacing );

  //Finally allocate
  orcArmy->Allocate();

  //Fill the image
  int bigOrcRadius = 2000;  //Orc army formation circus
  int smallOrcRadius = 3900;  //Orc haz big army
  int gobelinRadius = 4000; //Orc army haz gobelins 2
  ImageType::IndexType orcArmyCenter = { 10000, 10000 };
  
  typedef itk::ImageRegionIterator<ImageType> IteratorType;
  IteratorType orcGenerator( orcArmy, orcArmy->GetLargestPossibleRegion() );
  for (orcGenerator.GoToBegin(); !orcGenerator.IsAtEnd(); ++orcGenerator)
    {
    ImageType::IndexType currentIndex = orcGenerator.GetIndex();

    int xDist = currentIndex[0] - orcArmyCenter[0];
    int yDist = currentIndex[1] - orcArmyCenter[1];
    int distance = xDist*xDist + yDist*yDist;

    if (distance < bigOrcRadius*bigOrcRadius)
      {
      orcGenerator.Set( 255 );
      }
    else if (distance < smallOrcRadius*smallOrcRadius)
      {
      orcGenerator.Set( 200 ); //small orc not so strong
      }
    else if (distance < gobelinRadius*gobelinRadius)
      {
      orcGenerator.Set( 100 ); //Gobelin halve value of orc
                               //Gobelin go dye first !
                               // niark ! niark ! niark !
      }
    }

  //Take it out
  typedef itk::ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer mordor = WriterType::New();
  mordor->SetInput( orcArmy );
  mordor->SetFileName( "./OrcArmy.png" );
  mordor->Update();

  //
  //Now Fast march dilate it
  // (No alive seeds)
  //

  typedef itk::FastMarchingImageFilter<FloatImageType, FloatImageType>  FastMarchingFilterType;
  typedef itk::BinaryThresholdImageFilter<FloatImageType, ImageType>  ThresholdingFilterType;

  typedef FastMarchingFilterType::NodeContainer  NodeContainer;
  typedef FastMarchingFilterType::NodeType NodeType;
  NodeType node;

  typedef itk::ImageRegionConstIterator<ImageType>  ConstIteratorType;
  ConstIteratorType it( orcArmy, orcArmy->GetLargestPossibleRegion() );

  if (true) //Astuce to delete the fast marching so we can study the memory consumption
    {

    //Start Chrono
    time_t t = time(NULL); //Get Start Time

    //Instantiations
    FastMarchingFilterType::Pointer growOrcs = FastMarchingFilterType::New();
    NodeContainer::Pointer seeds = NodeContainer::New();
    seeds->Initialize();


    node.SetValue( 0.0 );  // seed value is 0 for all of them
                           //because these are all starting nodes

    unsigned int numberOfSeeds = 0;
    for ( it.GoToBegin(); !it.IsAtEnd(); ++it )
      {
      if ( it.Get() > 0 )
        {
        node.SetIndex( it.GetIndex() );
        seeds->InsertElement( numberOfSeeds++, node );
        }
      }

    //Only Trial points the first time
    growOrcs->SetTrialPoints( seeds );
    growOrcs->SetInput( NULL );
    growOrcs->SetSpeedConstant( 1.0 );  // to solve a simple Eikonal equation
    growOrcs->SetOutputSize( orcArmy->GetBufferedRegion().GetSize() );
    growOrcs->SetOutputRegion( orcArmy->GetBufferedRegion() );
    growOrcs->SetOutputSpacing( orcArmy->GetSpacing() );
    growOrcs->SetOutputOrigin( orcArmy->GetOrigin() );
    growOrcs->SetStoppingValue( 800 );

    try
      {
      growOrcs->Update();
      }
    catch ( itk::ExceptionObject & excep )
      {
      std::cerr << "Exception caught !" << std::endl;
      std::cerr << excep << std::endl; 
      }

    ThresholdingFilterType::Pointer threshold = ThresholdingFilterType::New();
    threshold->SetInput( growOrcs->GetOutput() );
    threshold->SetLowerThreshold( 0.0 );
    threshold->SetUpperThreshold( 700.0 );
    threshold->SetOutsideValue( 0 );
    threshold->SetInsideValue( 255 );
    threshold->Update();

    //Stop timer and print info
    time_t orcArmyGrowthTime = time(NULL) - t;
    std::cout<< "The orc army grew in " << orcArmyGrowthTime <<std::endl;
    std::cout<< "There was " << numberOfSeeds << " trials points" <<std::endl;

    //Write out images
    typedef itk::CastImageFilter<FloatImageType, ImageType>  CastFilterType;
    CastFilterType::Pointer cast = CastFilterType::New();
    cast->SetInput( growOrcs->GetOutput() );

    mordor->SetInput( cast->GetOutput() );
    mordor->SetFileName( "./OrcArmyOnSteroids_CastToUnsignedChar.png" );
    mordor->Update();

    mordor->SetInput( threshold->GetOutput() );
    mordor->SetFileName( "./OrcArmyOnSteroids.png" );
    mordor->Update();
    }

  //
  //Now Fast march dilate it
  // (With alive seeds)
  //

  if (true) //First, separate big orcs and the rest
    {
    //Start Chrono
    time_t t = time(NULL); //Get Start Time

    //Instantiations
    FastMarchingFilterType::Pointer growOrcsWithMagic = FastMarchingFilterType::New();
    NodeContainer::Pointer trialSeeds = NodeContainer::New();
    NodeContainer::Pointer aliveSeeds = NodeContainer::New();
    trialSeeds->Initialize();
    aliveSeeds->Initialize();

    node.SetValue( 0.0 );  // seed value is 0 for all of them
                           //because these are all starting nodes

    int numberOfAliveSeeds = 0;
    int numberOfTrialSeeds = 0;
    for ( it.GoToBegin(); !it.IsAtEnd(); ++it )
      {
      if (it.Get() > 0)
        {
        node.SetIndex( it.GetIndex() );

        if (it.Get() > 200) //Yes
          {
          aliveSeeds->InsertElement( numberOfAliveSeeds++, node );
          }
        else //Nop
          {
          trialSeeds->InsertElement( numberOfTrialSeeds++, node );
          }
        }
      }

    //Trial And Alive seeds
    growOrcsWithMagic->SetTrialPoints( trialSeeds );
    growOrcsWithMagic->SetAlivePoints( aliveSeeds );
    growOrcsWithMagic->SetInput( NULL );
    growOrcsWithMagic->SetSpeedConstant( 1.0 );  // to solve a simple Eikonal equation
    growOrcsWithMagic->SetOutputSize( orcArmy->GetBufferedRegion().GetSize() );
    growOrcsWithMagic->SetOutputRegion( orcArmy->GetBufferedRegion() );
    growOrcsWithMagic->SetOutputSpacing( orcArmy->GetSpacing() );
    growOrcsWithMagic->SetOutputOrigin( orcArmy->GetOrigin() );
    growOrcsWithMagic->SetStoppingValue( 800 );

    try
      {
      growOrcsWithMagic->Update();
      }
    catch ( itk::ExceptionObject & excep )
      {
      std::cerr << "Exception caught !" << std::endl;
      std::cerr << excep << std::endl; 
      }

    ThresholdingFilterType::Pointer thresholdWithMagic = ThresholdingFilterType::New();
    thresholdWithMagic->SetInput( growOrcsWithMagic->GetOutput() );
    thresholdWithMagic->SetLowerThreshold( 0.0 );
    thresholdWithMagic->SetUpperThreshold( 700.0 );
    thresholdWithMagic->SetOutsideValue( 0 );
    thresholdWithMagic->SetInsideValue( 255 );
    thresholdWithMagic->Update();

    //Stop timer and print info
    time_t orcArmyGrowthTimeWithMagic = time(NULL) - t;
    std::cout<< "The orc army grew in " << orcArmyGrowthTimeWithMagic
             << " (it had magic)" <<std::endl;
    std::cout<< "There was " << numberOfTrialSeeds << " trials points" <<std::endl;
    std::cout<< "And was " << numberOfAliveSeeds << " alive points" <<std::endl;

    //Write out images
    typedef itk::CastImageFilter<FloatImageType, ImageType>  CastFilterType;
    CastFilterType::Pointer cast = CastFilterType::New();
    cast->SetInput( growOrcsWithMagic->GetOutput() );

    mordor->SetInput( cast->GetOutput() );
    mordor->SetFileName( "./OrcArmyOnSteroidsWithMagics_CastToUnsignedChar.png" );
    mordor->Update();

    mordor->SetInput( thresholdWithMagic->GetOutput() );
    mordor->SetFileName( "./OrcArmyOnSteroidsWithMagic.png" );
    mordor->Update();
    }

  if (true) //First, separate orcs from gobelin (ony outside layer
    {
    //Start Chrono
    time_t t = time(NULL); //Get Start Time

    //Instantiations
    FastMarchingFilterType::Pointer growOrcsWithDarkMagic = FastMarchingFilterType::New();
    NodeContainer::Pointer trialSeeds = NodeContainer::New();
    NodeContainer::Pointer aliveSeeds = NodeContainer::New();
    trialSeeds->Initialize();
    aliveSeeds->Initialize();

    node.SetValue( 0.0 );  // seed value is 0 for all of them
                           //because these are all starting nodes

    int numberOfAliveSeeds = 0;
    int numberOfTrialSeeds = 0;
    for ( it.GoToBegin(); !it.IsAtEnd(); ++it )
      {
      if (it.Get() > 0)
        {
        node.SetIndex( it.GetIndex() );

        if (it.Get() > 100) //Yes
          {
          aliveSeeds->InsertElement( numberOfAliveSeeds++, node );
          }
        else //Nop
          {
          trialSeeds->InsertElement( numberOfTrialSeeds++, node );
          }
        }
      }

    //Trial And Alive seeds
    growOrcsWithDarkMagic->SetTrialPoints( trialSeeds );
    growOrcsWithDarkMagic->SetAlivePoints( aliveSeeds );
    growOrcsWithDarkMagic->SetInput( NULL );
    growOrcsWithDarkMagic->SetSpeedConstant( 1.0 );  // to solve a simple Eikonal equation
    growOrcsWithDarkMagic->SetOutputSize( orcArmy->GetBufferedRegion().GetSize() );
    growOrcsWithDarkMagic->SetOutputRegion( orcArmy->GetBufferedRegion() );
    growOrcsWithDarkMagic->SetOutputSpacing( orcArmy->GetSpacing() );
    growOrcsWithDarkMagic->SetOutputOrigin( orcArmy->GetOrigin() );
    growOrcsWithDarkMagic->SetStoppingValue( 800 );

    try
      {
      growOrcsWithDarkMagic->Update();
      }
    catch ( itk::ExceptionObject & excep )
      {
      std::cerr << "Exception caught !" << std::endl;
      std::cerr << excep << std::endl; 
      }

    ThresholdingFilterType::Pointer thresholdWithDarkMagic = ThresholdingFilterType::New();
    thresholdWithDarkMagic->SetInput( growOrcsWithDarkMagic->GetOutput() );
    thresholdWithDarkMagic->SetLowerThreshold( 0.0 );
    thresholdWithDarkMagic->SetUpperThreshold( 700.0 );
    thresholdWithDarkMagic->SetOutsideValue( 0 );
    thresholdWithDarkMagic->SetInsideValue( 255 );
    thresholdWithDarkMagic->Update();

    //Stop timer and print info
    time_t orcArmyGrowthTimeWithMagic = time(NULL) - t;
    std::cout<< "The orc army grew in " << orcArmyGrowthTimeWithMagic
             << " (it had DARK magic !!)" <<std::endl;
    std::cout<< "There was " << numberOfTrialSeeds << " trials points" <<std::endl;
    std::cout<< "And was " << numberOfAliveSeeds << " alive points" <<std::endl;

    //Write out images
    typedef itk::CastImageFilter<FloatImageType, ImageType>  CastFilterType;
    CastFilterType::Pointer cast = CastFilterType::New();
    cast->SetInput( growOrcsWithDarkMagic->GetOutput() );

    mordor->SetInput( cast->GetOutput() );
    mordor->SetFileName( "./OrcArmyOnSteroidsWithDarkMagic_CastToUnsignedChar.png" );
    mordor->Update();

    mordor->SetInput( thresholdWithDarkMagic->GetOutput() );
    mordor->SetFileName( "./OrcArmyOnSteroidsWithDarkMagic.png" );
    mordor->Update();
    }

  return EXIT_SUCCESS;
}