#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#include "Data/Table.h"
#include "Data/Zone.h"

@interface ZoneTests : XCTestCase
@end

@implementation ZoneTests

- (void)setUp {
    [super setUp];

    AG::data::table::ensure_shared();
}

// MARK: Zones

- (void)testZoneLifecycle {
    //    auto zone = AG::data::zone();
    //    XCTAssertEqual(zone.info().zone_id(), 1);
    //
    //    for (auto page : zone.pages()) {
    //        XCTFail(@"New zone shouldn't contain any pages");
    //    }
    //
    //    zone.clear();
}

// MARK: Alloc bytes

- (void)testAllocBytes {

    auto zone = AG::data::zone();

    auto bytes = zone.alloc_bytes(0x10, 3);
    XCTAssert(bytes != nullptr);
}

- (void)testAllocBytesWithExistingPage {
}

- (void)testAllocBytesWithExistingPageInsufficientCapacity {
}

@end
